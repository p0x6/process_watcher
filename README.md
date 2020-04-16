# process_watcher

NOTE: WINDOWS ONLY

process_watcher is a nodejs c++ addon for watching for process starting by using the WMI.

## Installation

```
$ npm install --save @p0x6/process_watcher
```

## Usage

```javascript
const process_watcher = require('@p0x6/process_watcher');

const notepadListener = () => {
    console.log('notepad opened.');
}

const calculatorListener = () => {
    console.log('calculator opened.');
}

const obj = new process_watcher.ProcessListener();

obj.addListener('notepad.exe',  notepadListener);
obj.addListener('calculator.exe', calculatorListener);
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)