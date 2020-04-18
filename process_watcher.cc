#include "process_watcher.h"
#include <thread>
#include <chrono>
#include <windows.h>
#include <tlhelp32.h>

Nan::Persistent<v8::Function> process_watcher_t::constructor;

process_watcher_t::process_watcher_t() {}

process_watcher_t::~process_watcher_t() {}

bool IsProcessRunning(const char *processName) {
    bool exists = false;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry))
        while (Process32Next(snapshot, &entry))
            if (!strcmp(entry.szExeFile, processName))
                exists = true;

    CloseHandle(snapshot);
    return exists;
}

void process_watcher_t::Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::Context> context = exports->CreationContext();
    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("ProcessListener").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "addListener", add_listener);
    Nan::SetPrototypeMethod(tpl, "removeListener", remove_listener);

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
    if(info.Length() != 2 || !(info[0]->IsArray()) || !(info[1]->IsFunction())){
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "incorrect arguments set").ToLocalChecked()));
    }
    process_watcher_t* obj = ObjectWrap::Unwrap<process_watcher_t>(info.Holder());
    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(info[0]);
    for (int i = 0; i < arr->Length(); ++i) {
        v8::Local<v8::String> process_name = v8::Local<v8::String>::Cast(arr->Get(isolate->GetCurrentContext(),i).ToLocalChecked());
        v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info[1]);
        v8::String::Utf8Value str(isolate, process_name);
        std::string cpp_str(*str);
        if (obj->registered_callbacks_.end() != obj->registered_callbacks_.find(cpp_str)) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, (std::string("listener for \"") + cpp_str + "\" is already registered").c_str()).ToLocalChecked()));
        }
        CComPtr<EventSink> sync = CreationEvent::registerCreationCallback(isolate, cpp_str, cb);
        obj->registered_callbacks_.insert(std::make_pair(cpp_str, sync));
        if (IsProcessRunning(*str)) {
            v8::Local<v8::Value> args[1];
            args[0] = v8::String::NewFromUtf8(isolate, cpp_str.c_str()).ToLocalChecked();
            v8::Local<v8::Function>::New(isolate, cb)->Call(isolate->GetCurrentContext(), v8::Null(isolate), 1, args);
         }
    }
    info.GetReturnValue().Set(Nan::New(0));
}

void process_watcher_t::remove_listener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    if (info.Length() != 1 || !(info[0]->IsArray())) {
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "incorrect arguments set").ToLocalChecked()));
    }
    process_watcher_t* obj = ObjectWrap::Unwrap<process_watcher_t>(info.Holder());
    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(info[0]);
    for (int i = 0; i < arr->Length(); ++i) {
        v8::Local<v8::String> process_name = v8::Local<v8::String>::Cast(arr->Get(isolate->GetCurrentContext(), i).ToLocalChecked());
        v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info[1]);
        v8::String::Utf8Value str(isolate, process_name);
        std::string cpp_str(*str);
        auto it = obj->registered_callbacks_.find(cpp_str);
        if (obj->registered_callbacks_.end() == it) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, (std::string("listener for \"") + cpp_str + "\" is already registered").c_str()).ToLocalChecked()));
        }
        it->second->pSvc->CancelAsyncCall(it->second->pStubSink);
        obj->registered_callbacks_.erase(it);
    }
    info.GetReturnValue().Set(Nan::New(0));
}

CComPtr<EventSink> CreationEvent::registerCreationCallback(v8::Isolate* isolate, std::string& p, v8::Local < v8::Function> f) {
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
    return pSink;
}
