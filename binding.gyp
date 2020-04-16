{
  "targets": [
    {
      "target_name": "process_watcher",
      "sources": [ "watcher.cc", "process_watcher.cc" ],
      'conditions': [['OS != "win"', {'sources!': ['watcher.cc', 'process_watcher.cc']}]]
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "libraries": [ "wbemuuid.lib"]
    }
  ]
}
