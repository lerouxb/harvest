'use strict';

const Aedes = require('aedes');
const Logging = require('aedes-logging');
const Net = require('net');
const Persistence = require('aedes-persistence');
const Stats = require('aedes-stats');

const persistence = Persistence();
const aedes = Aedes({ persistence });
const server = Net.createServer(aedes.handle);
const port = 1883;

Logging({
  instance: aedes,
  server: server
});

Stats(aedes);

server.listen(port, function () {
  console.log('server started and listening on port ', port);
});
