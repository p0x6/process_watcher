# process_watcher

NOTE: WINDOWS ONLY. TESTED WITH NODE 12.18.X WITH ELECTRON 7+

process_watcher is a nodejs c++ addon for watching for process starting by using the WMI.

## Prerequisites

1) Visual Studio with C++ dev is installed
2) ATLBase with spectre fixes have been applied
3) Windows OS
4) Node 12+

## Installation

```
$ npm install --save @p0x6/process_watcher
```

## Usage

If the process is already running when listener is added, it will trigger the callback.

```javascript
const process_watcher = require('@p0x6/process_watcher');
const obj = new process_watcher.ProcessListener();

const listener = (process) => {
    console.log(process + ' opened.');
    obj.removeListener([process]);
}

obj.addListener(['notepad.exe', 'calculator.exe'],  listener);
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://github.com/p0x6/process_watcher/blob/master/LICENSE)
