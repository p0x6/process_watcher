var process_watcher = require('./index');

const notepadListener = () => {
    console.log('League Client opened.');
}

const calculatorListener = () => {
    console.log('League Game opened.');
}

var obj = new process_watcher.ProcessListener();

obj.addListener('LeagueClient.exe',  notepadListener);
obj.addListener('League of Legends.exe', calculatorListener);

process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.on('data', process.exit.bind(process, 0));

