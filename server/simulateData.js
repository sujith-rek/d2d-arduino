const axios = require('./node_modules/axios/index.d.cts');

setInterval(() => {
    const temperature = (Math.random() * 10 + 20).toFixed(2); // Simulated temperature
    const humidity = (Math.random() * 20 + 30).toFixed(2); // Simulated humidity
    const CO2 = (Math.random() * 300 + 400).toFixed(2); // Simulated CO2

    axios.post('http://localhost:3000/sensor-data', {
        temperature,
        humidity,
        CO2
    })
        .then(response => {
            console.log('Data sent to server');
        })
        .catch(error => {
            console.log('Error:', error);
        });

}, 3000); // Sends data every 3 seconds