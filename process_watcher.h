#ifndef PROCESS_WATCHER_H
#define PROCESS_WATCHER_H

#include <nan.h>
#include <map>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <atlbase.h>

struct Work {
    uv_work_t  request;
    std::string s;
    v8::Persistent<v8::Function>* callback;
};

static void WorkAsync(uv_work_t* req){
    Sleep(30);
}

static void WorkAsyncComplete(uv_work_t* req, int status){
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);
    Work* work = static_cast<Work*>(req->data);
    v8::Local<v8::Value> args[1];
    args[0] = v8::String::NewFromUtf8(isolate, work->s.c_str()).ToLocalChecked();
    v8::Local<v8::Function>::New(isolate, *(work->callback))->Call(isolate->GetCurrentContext(), v8::Null(isolate), 1, args);
//    work->callback->Reset();
    delete work;
}

class EventSink : public IWbemObjectSink {
    //friend  CComPtr<EventSink> CreationEvent::registerCreationCallback(v8::Isolate* isolate, std::string& p, v8::Local < v8::Function> f);
public:

    CComPtr<IWbemServices> pSvc;
    CComPtr<IWbemObjectSink> pStubSink;
    LONG m_IRef;
    std::string proc_name;
    Nan::Persistent < v8::Function> callback_;
    v8::Isolate* isolate_;
//    CreationEvent::TNotificationFunc m_callback;

public:
    EventSink(v8::Isolate* isolate, std::string& p, v8::Local< v8::Function> f) :m_IRef(0), isolate_(isolate), proc_name(p) {
        callback_.Reset(f);
    }
    ~EventSink() {
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() {
        return InterlockedIncrement(&m_IRef);
    }

    virtual ULONG STDMETHODCALLTYPE Release() {
        LONG IRef = InterlockedDecrement(&m_IRef);
        if (IRef == 0)
            delete this;
        return IRef;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
            *ppv = (IWbemObjectSink*)this;
            AddRef();
            return WBEM_S_NO_ERROR;
        }
        else return E_NOINTERFACE;
    }

    virtual HRESULT STDMETHODCALLTYPE Indicate(LONG lObjectCount, IWbemClassObject __RPC_FAR* __RPC_FAR* apObjArray) {
        Work* w = new Work();
        w->request.data = w;
        w->callback = &callback_;
        w->s = proc_name;
        uv_queue_work(uv_default_loop(), &w->request, WorkAsync, WorkAsyncComplete);
        return WBEM_S_NO_ERROR;
    }
    virtual HRESULT STDMETHODCALLTYPE SetStatus(LONG IFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR* pObjParam) {
        return WBEM_S_NO_ERROR;
    }
};

namespace CreationEvent {
    CComPtr<EventSink> registerCreationCallback(v8::Isolate* isolate, std::string& p, v8::Local < v8::Function> f);
}


class process_watcher_t : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports);

 private:
  explicit process_watcher_t();
  ~process_watcher_t();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void add_listener(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void remove_listener(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> constructor;

  std::map<std::string, CComPtr<EventSink> > registered_callbacks_;
};

bool IsProcessRunning(const char *processName);

#endif
