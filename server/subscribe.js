#!/usr/bin/env node
'use strict';

require('dotenv').config();

const { MongoClient } = require("mongodb");
const MQTT = require('async-mqtt');

const DATABASE_NAME = 'harvest';
const SENSOR_ID = 'greenhouse';


const internals = {
    handlers: {}
};

const mongo = new MongoClient(process.env.MONGO_URI, {
    useNewUrlParser: true,
    useUnifiedTopology: true
});

run().catch(async (err) => {

    console.error(err);
    await internals.abort();
    process.exit(1);
});


async function run () {

    await mongo.connect();

    const mqtt = await MQTT.connectAsync("tcp://localhost:1883", { clean: false, clientId: 'subscriber' });
    mqtt.on('message', handleMessage);
    await mqtt.subscribe('temperature', { qos: 1 });
    await mqtt.subscribe('humidity', { qos: 1 });
    await mqtt.subscribe('voltage', { qos: 1 });
    // TODO: image
};


async function handleMessage (topic, message) {

    try {
        await internals.handlers[topic]({ topic, message });
    }
    catch (err) {

        console.error(err);
        await internals.abort();
        process.exit(1);
    }
};


internals.date = function () {

    const date = new Date();
    return date.toISOString(); // undecided on format
};


internals.handlers.temperature = async function ({ message }) {


    console.log(internals.date(), `${message}C`);
    await internals.insert('temperature', SENSOR_ID, message);
};


internals.handlers.humidity = async function ({ message}) {

    console.log(internals.date(), `${message}%`);
    await internals.insert('humidity', SENSOR_ID, message);
};


internals.handlers.voltage = async function ({ message}) {

    console.log(internals.date(), `${message}V`);
    await internals.insert('voltage', SENSOR_ID, message);
};


internals.insert = async function (collectionName, sensor, value) {

    value = value * 1; // cast to a number

    // one database for everything
    const database = mongo.db(DATABASE_NAME);

    // one collection per value we track
    const collection = database.collection(collectionName);

    // one doc per day and sensor
    const timestamp = (new Date()).toISOString();
    const date = timestamp.split('T')[0];
    const _id = `${date}-${sensor}`;

    const query = { _id };

    const update = {
        $push: { values: { t: timestamp, v: value } },
        $setOnInsert: {
            _id,
            date,
            sensor
        }
    };

    const result = await collection.updateOne(query, update, { upsert: true });
};


internals.abort = async function () {

    await mongo.close();
};
