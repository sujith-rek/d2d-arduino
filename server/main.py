from fastapi import FastAPI, BackgroundTasks
from pydantic import BaseModel
import firebase_admin
from firebase_admin import credentials, db
from typing import List, Dict
import math
from datetime import datetime, timedelta
import asyncio

cred = credentials.Certificate("firebase-credentials.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://aqitracker-29685-default-rtdb.asia-southeast1.firebasedatabase.app/'  # Replace with your Firebase URL
})

app = FastAPI()

# Reference to your Firebase database
ref = db.reference('sensor_data')

app = FastAPI()

# Constants for room and environment
ROOM_VOLUME = 320000  # Volume of room in cubic feet
OUTDOOR_CO2 = 400  # Outdoor CO2 concentration in ppm
METABOLIC_RATE = 58  # Average metabolic rate in W/m2 (seated)
CLOTHING_FACTOR = 1.0  # Assumed clothing factor

# Global variables for data storage
sensor_data: List[Dict] = []
INTERVAL_TIME = 3  # data received every 3 seconds
TOTAL_DURATION = 15  # total time for averaging

class SensorData(BaseModel):
    temp: float
    hum: float
    co2: float
    co: float
    pm25: float
    pm10: float

def calculate_average(data: List[Dict]) -> Dict:
    """Calculate average of sensor readings"""
    if not data:
        return {
            "temp": 0, 
            "hum": 0, 
            "co2": 0,
            "co": 0,
            "pm25": 0,
            "pm10": 0
        }
    
    total = {
        "temp": sum(d["temp"] for d in data),
        "hum": sum(d["hum"] for d in data),
        "co2": sum(d["co2"] for d in data),
        "co": sum(d["co"] for d in data),
        "pm25": sum(d["pm25"] for d in data),
        "pm10": sum(d["pm10"] for d in data)
    }
    
    return {
        "temp": round(total["temp"] / len(data), 2),
        "hum": round(total["hum"] / len(data), 2),
        "co2": round(total["co2"] / len(data), 2),
        "co": round(total["co"] / len(data), 2),
        "pm25": round(total["pm25"] / len(data), 2),
        "pm10": round(total["pm10"] / len(data), 2)
    }

def calculate_vr(avg_co2: float) -> float:
    """Calculate Ventilation Rate (VR)"""
    air_change_rate = math.log((avg_co2 - OUTDOOR_CO2) / avg_co2)
    ventilation_rate = (air_change_rate * ROOM_VOLUME * (avg_co2 - OUTDOOR_CO2) / avg_co2) * 60
    return round(ventilation_rate, 2)

def calculate_pmv(avg_temp: float, avg_hum: float) -> float:
    """Calculate Predicted Mean Vote (PMV)"""
    Ta = avg_temp
    Pa = avg_hum * 10
    M = METABOLIC_RATE
    W = 0
    Fcl = CLOTHING_FACTOR
    Tcl = Ta
    Tr = Ta
    hc = 10.45

    pmv = (0.303 * math.exp(-0.036 * M) + 0.028) * (
        (M - W) - 3.05 * pow(10, -3) * (5733 - 6.99 * (M - W)) -
        0.42 * (M - W) - 58.15 - 1.7 * pow(10, -5) * (34 - Ta) -
        3.96 * pow(10, -8) * Fcl * (pow(Tcl, 4) - pow(Tr, 4)) -
        Fcl * hc * (Tcl - Ta)
    )
    
    return round(pmv, 2)

async def send_to_firebase(data: Dict):
    """Send data to Firebase Realtime Database"""
    try:
        # Create a timestamp for the data
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Create a new child in the sensor_data node with timestamp
        new_ref = ref.child(timestamp.replace(" ", "_"))
        
        # Set the data
        new_ref.set(data)
        
        print(f"Data sent to Firebase successfully at {timestamp}")
        
        # Keep only last 24 hours of data
        # Delete old data
        cutoff_time = (datetime.now() - timedelta(days=1)).strftime("%Y-%m-%d %H:%M:%S")
        old_data_ref = ref.order_by_key().end_at(cutoff_time)
        old_data = old_data_ref.get()
        if old_data:
            for key in old_data:
                ref.child(key).delete()
                print(f"Deleted old data: {key}")
                
    except Exception as e:
        print(f"Error sending data to Firebase: {e}")

def calculate_aqi(co: float, pm25: float, pm10: float, co2: float) -> Dict:
    """
    Calculate Air Quality Index based on EPA standards
    All input values should be in µg/m³ for PM2.5 and PM10, ppm for CO and CO2
    """
    def calculate_pm25_aqi(pm25: float) -> tuple:
        """Calculate AQI for PM2.5"""
        if pm25 <= 12.0:
            return linear_scale(pm25, 0, 12.0, 0, 50), "Good"
        elif pm25 <= 35.4:
            return linear_scale(pm25, 12.1, 35.4, 51, 100), "Moderate"
        elif pm25 <= 55.4:
            return linear_scale(pm25, 35.5, 55.4, 101, 150), "Unhealthy for Sensitive Groups"
        elif pm25 <= 150.4:
            return linear_scale(pm25, 55.5, 150.4, 151, 200), "Unhealthy"
        elif pm25 <= 250.4:
            return linear_scale(pm25, 150.5, 250.4, 201, 300), "Very Unhealthy"
        else:
            return linear_scale(pm25, 250.5, 500.4, 301, 500), "Hazardous"

    def calculate_pm10_aqi(pm10: float) -> tuple:
        """Calculate AQI for PM10"""
        if pm10 <= 54:
            return linear_scale(pm10, 0, 54, 0, 50), "Good"
        elif pm10 <= 154:
            return linear_scale(pm10, 55, 154, 51, 100), "Moderate"
        elif pm10 <= 254:
            return linear_scale(pm10, 155, 254, 101, 150), "Unhealthy for Sensitive Groups"
        elif pm10 <= 354:
            return linear_scale(pm10, 255, 354, 151, 200), "Unhealthy"
        elif pm10 <= 424:
            return linear_scale(pm10, 355, 424, 201, 300), "Very Unhealthy"
        else:
            return linear_scale(pm10, 425, 604, 301, 500), "Hazardous"

    def calculate_co_aqi(co: float) -> tuple:
        """Calculate AQI for CO (input in ppm)"""
        if co <= 4.4:
            return linear_scale(co, 0, 4.4, 0, 50), "Good"
        elif co <= 9.4:
            return linear_scale(co, 4.5, 9.4, 51, 100), "Moderate"
        elif co <= 12.4:
            return linear_scale(co, 9.5, 12.4, 101, 150), "Unhealthy for Sensitive Groups"
        elif co <= 15.4:
            return linear_scale(co, 12.5, 15.4, 151, 200), "Unhealthy"
        elif co <= 30.4:
            return linear_scale(co, 15.5, 30.4, 201, 300), "Very Unhealthy"
        else:
            return linear_scale(co, 30.5, 50.4, 301, 500), "Hazardous"

    def calculate_co2_level(co2: float) -> tuple:
        """
        Determine CO2 level category
        While CO2 isn't part of AQI, we provide a categorization for reference
        """
        if co2 <= 700:
            return 0, "Excellent"
        elif co2 <= 1000:
            return 1, "Good"
        elif co2 <= 2000:
            return 2, "Moderate"
        elif co2 <= 5000:
            return 3, "Poor"
        else:
            return 4, "Hazardous"

    def linear_scale(value: float, low_conc: float, high_conc: float, low_aqi: float, high_aqi: float) -> float:
        """Calculate AQI value using linear interpolation"""
        return round(((high_aqi - low_aqi) / (high_conc - low_conc)) * (value - low_conc) + low_aqi)

    # Calculate AQI for each pollutant
    pm25_aqi, pm25_category = calculate_pm25_aqi(pm25)
    pm10_aqi, pm10_category = calculate_pm10_aqi(pm10)
    co_aqi, co_category = calculate_co_aqi(co)
    co2_value, co2_category = calculate_co2_level(co2)

    # Determine overall AQI (highest of PM2.5, PM10, and CO)
    pollutant_aqis = [
        ("PM2.5", pm25_aqi, pm25_category),
        ("PM10", pm10_aqi, pm10_category),
        ("CO", co_aqi, co_category)
    ]

    # Find the dominant pollutant (highest AQI)
    dominant_pollutant = max(pollutant_aqis, key=lambda x: x[1])

    return {
        "aqi": dominant_pollutant[1],
    }

async def process_sensor_data():
    """Background task to process sensor data periodically"""
    while True:
        if sensor_data:
            try:
                avg_data = calculate_average(sensor_data)
                print(f"Average Data: {avg_data}")

                # Calculate AQI
                aqi_data = calculate_aqi(
                    float(avg_data["co"]),
                    float(avg_data["pm25"]),
                    float(avg_data["pm10"]),
                    float(avg_data["co2"])
                )

                # Calculate additional metrics
                ventilation_rate = calculate_vr(float(avg_data["co2"]))
                pmv = calculate_pmv(float(avg_data["temp"]), float(avg_data["hum"]))

                # Prepare data for Firebase
                firebase_data = {
                    "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    "average_readings": avg_data,
                    "aqi_data": aqi_data,
                    "ventilation_rate": ventilation_rate,
                    "pmv": pmv
                }

                # Send to Firebase
                await send_to_firebase(firebase_data)

            except Exception as e:
                print(f"Error in processing sensor data: {e}")

        await asyncio.sleep(TOTAL_DURATION)

@app.post("/sensor-data")
async def receive_sensor_data(data: SensorData):
    """Endpoint to receive sensor data"""
    try:
        sensor_reading = {
            "temp": data.temp,
            "hum": data.hum,
            "co2": data.co2,
            "co": data.co,
            "pm25": data.pm25,
            "pm10": data.pm10
        }
        
        # Add new data to local storage
        sensor_data.append(sensor_reading)
        
        # Keep only last 15 seconds of data
        max_readings = TOTAL_DURATION // INTERVAL_TIME
        if len(sensor_data) > max_readings:
            sensor_data.pop(0)
        
        # Calculate current AQI
        current_aqi = calculate_aqi(
            data.co,
            data.pm25,
            data.pm10,
            data.co2
        )
        
        # Prepare immediate response data
        response_data = {
            "message": "Data received successfully",
            "current_readings": sensor_reading,
            "aqi_data": current_aqi,
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }
        
        # Send immediate reading to Firebase
        await send_to_firebase({
            "instant_reading": response_data
        })
        
        return response_data

    except Exception as e:
        print(f"Error processing request: {e}")
        return {"error": str(e)}, 500

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=3000)