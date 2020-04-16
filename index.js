'use strict';

// Only works on Windows!
if (process.platform !== 'win32') return;

const process_watcher = require('./build/Release/process_watcher.node');

module.exports = process_watcher;