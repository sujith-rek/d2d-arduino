const express = require('express');
const app = express();
const port = 3000;

app.use(express.json());

let sensorData = [];
let intervalTime = 3; // data received every 3 seconds
let totalDuration = 15; // total time for averaging

// Constants for room and environment
const roomVolume = 320000; // Volume of room in cubic feet
const outdoorCO2 = 400; // Outdoor CO2 concentration in ppm
const metabolicRate = 58; // Average metabolic rate in W/m2 (seated)
const clothingFactor = 1.0; // Assumed clothing factor

// Function to calculate average
const calculateAverage = (data) => {
    const sum = data.reduce((acc, curr) => {
        return {
            temperature: acc.temperature + curr.temperature,
            humidity: acc.humidity + curr.humidity,
            CO2: acc.CO2 + curr.CO2
        };
    }, { temperature: 0, humidity: 0, CO2: 0 });

    return {
        temperature: (sum.temperature / data.length).toFixed(2),
        humidity: (sum.humidity / data.length).toFixed(2),
        CO2: (sum.CO2 / data.length).toFixed(2)
    };
};

// Function to calculate Ventilation Rate (VR)
const calculateVR = (avgCO2) => {
    const airChangeRate = Math.log((avgCO2 - outdoorCO2) / avgCO2); // Approximation for air change rate λ
    const ventilationRate = (airChangeRate * roomVolume * (avgCO2 - outdoorCO2) / avgCO2) * 60; // CFM
    return ventilationRate.toFixed(2);
};

// Function to calculate Predicted Mean Vote (PMV)
const calculatePMV = (avgTemperature, avgHumidity) => {
    const Ta = avgTemperature; // Air temperature
    const Pa = avgHumidity * 10; // Water vapor pressure

    // PMV formula components
    const M = metabolicRate; // Metabolic rate
    const W = 0; // External work (assumed 0 for seated)
    const Fcl = clothingFactor; // Clothing factor
    const Tcl = Ta; // Clothing temperature (assumed same as air temp)
    const Tr = Ta; // Radiant temperature (assumed same as air temp)
    const hc = 10.45; // Heat transfer coefficient

    // PMV equation
    const PMV = (0.303 * Math.exp(-0.036 * M) + 0.028) *
        ((M - W) - 3.05 * Math.pow(10, -3) * (5733 - 6.99 * (M - W)) -
            0.42 * (M - W) - 58.15 - 1.7 * Math.pow(10, -5) * (34 - Ta) -
            3.96 * Math.pow(10, -8) * Fcl * (Math.pow(Tcl, 4) - Math.pow(Tr, 4)) -
            Fcl * hc * (Tcl - Ta));

    return PMV.toFixed(2);
};

// Endpoint to receive sensor data from Arduino
app.post('/sensor-data', (req, res) => {
    const { temperature, humidity, CO2 } = req.body;

    // Parse string data to numbers
    const parsedData = {
        temperature: parseFloat(temperature),
        humidity: parseFloat(humidity),
        CO2: parseFloat(CO2)
    };

    // Store data received
    sensorData.push(parsedData);

    // Only store data for the last 15 seconds
    if (sensorData.length > totalDuration / intervalTime) {
        sensorData.shift(); // Remove old data
    }

    console.log('Data received:', parsedData);
    res.send('Data received');
});

// Periodically calculate average, VR, and PMV every 15 seconds
setInterval(() => {
    if (sensorData.length > 0) {
        const avgData = calculateAverage(sensorData);
        console.log('Average Data:', avgData);

        // Calculate VR
        const ventilationRate = calculateVR(avgData.CO2);
        console.log('Ventilation Rate (VR):', ventilationRate, 'CFM');

        // Calculate PMV
        const pmv = calculatePMV(avgData.temperature, avgData.humidity);
        console.log('Predicted Mean Vote (PMV):', pmv);

        // Save the calculations or send them for further analysis
        console.log('Calculations done');
    }
}, totalDuration * 1000);

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});
