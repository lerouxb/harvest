#!/usr/bin/env node
'use strict';

const MQTT = require('async-mqtt');

const internals = {};

run().catch((err) => {

    console.error(err);
    process.exit(1);
});


async function run () {

    const client = await MQTT.connectAsync("tcp://localhost:1883");
    await client.publish('temperature', '23', { qos: 1 });
    await client.publish('humidity', '58', { qos: 1 });
    await client.publish('voltage', '4.2', { qos: 1 });
    await client.end();
};
