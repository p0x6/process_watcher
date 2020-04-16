#include "process_watcher.h"
#include <thread>
#include <chrono>

Nan::Persistent<v8::Function> process_watcher_t::constructor;

process_watcher_t::process_watcher_t() {}

process_watcher_t::~process_watcher_t() {}

void process_watcher_t::Init(v8::Local<v8::Object> exports) {
  v8::Local<v8::Context> context = exports->CreationContext();
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("ProcessListener").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "addListener", add_listener);
  Nan::SetPrototypeMethod(tpl, "start", start);
  Nan::SetPrototypeMethod(tpl, "stop", stop);

  constructor.Reset(tpl->GetFunction(context).ToLocalChecked());
  exports->Set(context,
               Nan::New("ProcessListener").ToLocalChecked(),
               tpl->GetFunction(context).ToLocalChecked());
}

void process_watcher_t::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  if (info.IsConstructCall()) {
    process_watcher_t* obj = new process_watcher_t();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = {info[0]};
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(cons->NewInstance(context, argc, argv).ToLocalChecked());
  }
}

void process_watcher_t::add_listener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if(info.Length() != 2 || !(info[0]->IsString()) || !(info[1]->IsFunction())){
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "incorrect arguments set")));
  }
  process_watcher_t* obj = ObjectWrap::Unwrap<process_watcher_t>(info.Holder());
  v8::Local<v8::String> process_name = info[0]->ToString(isolate);
  v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info[1]);
  v8::String::Utf8Value str(isolate, process_name);
  std::string cpp_str(*str);
  if (!(obj->registered_callbacks_.insert(cpp_str).second)) {
      isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, (std::string("listener for \"") + cpp_str + "\" is already registered").c_str())));
  }
  CreationEvent::registerCreationCallback(isolate, cpp_str, cb);
  info.GetReturnValue().Set(Nan::New(0));
}

void process_watcher_t::start(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    {
        isolate->Exit();
        v8::Unlocker unlocker(isolate);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    v8::Locker locker(isolate);
    isolate->Enter();
}

void process_watcher_t::stop(const Nan::FunctionCallbackInfo<v8::Value>& info) {
}

void CreationEvent::registerCreationCallback(v8::Isolate* isolate, std::string& p, v8::Local < v8::Function> f) {
    CComPtr<IWbemLocator> pLoc;
    CoInitializeEx(0, COINIT_MULTITHREADED);
    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        std::cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << std::hex << hres << std::endl;
        throw std::exception("CreationEvent initialization failed");
    }
    CComPtr<EventSink> pSink(new EventSink(isolate, p, f));

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSink->pSvc);
    if (FAILED(hres)) {
        std::cout << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
        throw std::exception("CreationEvent initialization failed");
    }
    hres = CoSetProxyBlanket(pSink->pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        std::cout << "Coult not set proxy blanket, Error code =0x" << std::hex << hres << std::endl;
        throw std::exception("CreationEvent initialization failed");
    }

    CComPtr<IUnsecuredApartment> pUnsecApp;
    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, (void**)&pUnsecApp);
    CComPtr<IUnknown> pStubUnk;
    pUnsecApp->CreateObjectStub(pSink, &pStubUnk);
    pStubUnk->QueryInterface(IID_IWbemObjectSink, (void**)&pSink->pStubSink);

    char buffer[512];
    sprintf_s(buffer, "SELECT * FROM __InstanceCreationEvent WITHIN 1 WHERE TargetInstance ISA 'Win32_Process' AND TargetInstance.Name = '%s'", p.c_str());

    hres = pSink->pSvc->ExecNotificationQueryAsync(_bstr_t("WQL"), _bstr_t(buffer), WBEM_FLAG_SEND_STATUS, NULL, pSink->pStubSink);
    if (FAILED(hres)) {
        std::cout << "ExecNotificationQueryAsync failed with = 0x" << std::hex << hres << std::endl;
        throw std::exception("CreationEvent initialization failed");
    }
}
