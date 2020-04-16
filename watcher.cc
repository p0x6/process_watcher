#include <nan.h>
#include "process_watcher.h"

void InitAll(v8::Local<v8::Object> exports) {
  process_watcher_t::Init(exports);
}

NODE_MODULE(addon, InitAll)
