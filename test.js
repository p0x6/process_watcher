const process_watcher = require('./index');
const obj = new process_watcher.ProcessListener();

const listener = (process) => {
    console.log(process + ' opened.');
    obj.removeListener([process]);
}

obj.addListener(['LeagueClient.exe', 'League of Legends.exe'],  listener);

process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.on('data', process.exit.bind(process, 0));

