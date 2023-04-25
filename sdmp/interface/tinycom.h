/*
tiny subset of com(component object module). Choice of public domain or MIT-0. See license statements at the end of this file.
mr::tinycom - v0.1.0 - 2022-2-2

xuwei - xiaoxuweier@gmail.com
*/
#ifndef MOUNTAIN_RIPPER_TINYCOM_H_
#define MOUNTAIN_RIPPER_TINYCOM_H_
#include <cstdint>
#include <cassert>
#include <cstring>
#include <string>
#include <atomic>
#include <vector>
#include <algorithm>
#include <map>

typedef int32_t        TRESULT;
#define S_OK           static_cast<int32_t>(0L)
#define E_BOUNDS       static_cast<int32_t>(0x8000000BL)
#define E_NOTIMPL      static_cast<int32_t>(0x80004001L)
#define E_NOINTERFACE  static_cast<int32_t>(0x80004002L)
#define E_POINTER      static_cast<int32_t>(0x80004003L)
#define E_ABORT        static_cast<int32_t>(0x80004004L)
#define E_FAIL         static_cast<int32_t>(0x80004005L)
#define E_UNEXPECTED   static_cast<int32_t>(0x8000FFFFL)
#define E_ACCESSDENIED static_cast<int32_t>(0x80070005L)
#define E_HANDLE       static_cast<int32_t>(0x80070006L)
#define E_OUTOFMEMORY  static_cast<int32_t>(0x8007000EL)
#define E_INVALIDARG   static_cast<int32_t>(0x80070057L)
#define E_NOT_SET      static_cast<int32_t>(0x80070490L)
//#if !defined(WIN32) && !defined(WIN64)

//enum CLSCTX {
//  CLSCTX_INPROC_SERVER           = 0x1,
//  CLSCTX_INPROC_HANDLER          = 0x2,
//  CLSCTX_LOCAL_SERVER            = 0x4,
//  CLSCTX_REMOTE_SERVER           = 0x10,
//  CLSCTX_NO_CODE_DOWNLOAD        = 0x400,
//  CLSCTX_NO_CUSTOM_MARSHAL       = 0x1000,
//  CLSCTX_ENABLE_CODE_DOWNLOAD    = 0x2000,
//  CLSCTX_NO_FAILURE_LOG          = 0x4000,
//  CLSCTX_DISABLE_AAA             = 0x8000,
//  CLSCTX_ENABLE_AAA              = 0x10000,
//  CLSCTX_FROM_DEFAULT_CONTEXT    = 0x20000,
//  CLSCTX_ACTIVATE_32_BIT_SERVER  = 0x40000,
//  CLSCTX_ACTIVATE_64_BIT_SERVER  = 0x80000,
//  CLSCTX_ENABLE_CLOAKING         = 0x100000,
//  CLSCTX_APPCONTAINER            = 0x400000,
//  CLSCTX_ACTIVATE_AAA_AS_IU      = 0x800000,
//  CLSCTX_PS_DLL                  = 0x80000000
//};
//#define CLSCTX_ALL              (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)
//#endif

namespace mr::tinycom {

//#define USE_GUID_AS_STRING

#ifdef USE_GUID_AS_STRING
struct TGUID : public std::string {
public:

  using std::string::string;

  std::string ToString() const {
    std::string s;
    std::for_each(this->begin(), this->end(), [&s](const char& c) {
      if ('{' != c && '}' != c)
        s.push_back(std::tolower(c));
    });
    return s;
  }

  bool operator<(const TGUID& id) const {
    std::string self;
    std::for_each(this->begin(), this->end(), [&self](const char& c) {
      if ('{' != c && '}' != c)
        self.push_back(std::tolower(c));
    });

    std::string other;
    std::for_each(id.begin(), id.end(), [&other](const char& c) {
      if ('{' != c && '}' != c)
        other.push_back(std::tolower(c));
    });

    return self < other;
  }
};

inline bool IsEqualUuid(const TGUID& a, const TGUID& b) {
  if (a.empty() || b.empty())
    return false;

  std::string aa;
  std::for_each(a.begin(), a.end(), [&aa](const char& c) {
    if ('{' != c && '}' != c)
      aa.push_back(std::tolower(c));
  });

  std::string bb;
  std::for_each(b.begin(), b.end(), [&bb](const char& c) {
    if ('{' != c && '}' != c)
      bb.push_back(std::tolower(c));
  });

  if (aa.length() != bb.length())
    return false;

  return (0 == aa.compare(bb));
}
#else

struct TGUID{
    TGUID(){
        memset(this,0,sizeof(TGUID));
    }
    TGUID(const char* id){
        assert(id);
        uint32_t d1 = 0;
        uint16_t d2 = 0, d3 = 0;
        uint8_t d4[8] = {0};
        int err = sscanf(id, "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &d1,
                         &d2, &d3, &d4[0], &d4[1], &d4[2], &d4[3], &d4[4], &d4[5], &d4[6], &d4[7]);
        assert(11 == err);
        Data1 = d1;
        Data2 = d2;
        Data3 = d3;
        for (int i = 0; i < 8; i++)
          Data4[i] = d4[i];
    }
    TGUID(const TGUID& v){
        memcpy(this,&v,sizeof(TGUID));
    }
    std::string operator() (){
        char buf[64];
        int err = sprintf(buf, "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
                    Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5],
                    Data4[6], Data4[7]);
        assert(36 == err);
        return std::string(buf);
    }
    bool operator<(const TGUID& v) const {
        return (memcmp(this, &v, sizeof(TGUID)) < 0);
    }
    bool operator==(const TGUID& v) const {
        return (memcmp(this, &v, sizeof(TGUID)) == 0);
    }
    bool empty(){
        return (memcmp(this, &empty_guid(), sizeof(TGUID)) == 0);
    }
    static const TGUID& empty_guid(){
        static TGUID s_empty_guid;
        return s_empty_guid;
    }
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};

inline bool IsEqualUuid(const TGUID& a, const TGUID& b) {
  return (0 == memcmp(&a, &b, sizeof(TGUID)));
}
#endif

typedef mr::tinycom::TGUID  IID;
typedef mr::tinycom::TGUID  CLSID;

#define IIDOF(x) IID_##x
#define CLSIDOF(x) CLSID_##x
#define __t_uuidof(i) i::uuid()
#define __t_metadataof(i) i::metadata()

#define INTERFACE_INVALID_OFFSET (std::ptrdiff_t)(-1)

struct InterfaceEntry {
  const TGUID iid;
  const std::ptrdiff_t offset;
};


const char* const IID_IUnknown = "10000000-0000-0000-0000-000000000000";
class IUnknown{
public:
    virtual ~IUnknown(){}
    virtual int32_t AddRef() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t QueryInterface(const mr::tinycom::TGUID& iid,void **interface_ptr) = 0;

    static TGUID& uuid(){
        static TGUID iid_unknown(IID_IUnknown);
        return iid_unknown;
    }
};

static int32_t _QueryInterface(void* pThis, const InterfaceEntry* pEntries,
                               const TGUID& riid, void** ppvObject) {
  if (pThis == nullptr || pEntries == nullptr)
    return E_INVALIDARG;

  if (ppvObject == nullptr)
    return E_POINTER;

  if (IsEqualUuid(IID_IUnknown, riid)) {
    IUnknown* pUnk = (IUnknown*)((std::intptr_t)pThis + pEntries->offset);
    pUnk->AddRef();
    *ppvObject = pUnk;
    return S_OK;
  }

  int32_t error;

  for (;; pEntries++) {
    if (INTERFACE_INVALID_OFFSET == pEntries->offset) {
      error = E_NOINTERFACE;
      break;
    }

    if (IsEqualUuid(pEntries->iid, riid)) {
      IUnknown* pUnk = (IUnknown*)((std::intptr_t)pThis + pEntries->offset);
      pUnk->AddRef();
      *ppvObject = pUnk;
      return S_OK;
    }
  }

  *ppvObject = nullptr;

  return error;
}

template <typename ref_counter>
class ComObjectBase{
public:
    ComObjectBase(){
        refcount_ = 0;
    }

    int32_t InternalAddRef() {
        refcount_++;
        return refcount_;
    }
    int32_t InternalRelease() {
        refcount_--;
        return refcount_;
    }
    static int32_t InternalQueryInterface(void* pThis, const InterfaceEntry* pEntries,
                                   const TGUID& riid, void** interface_ptr) {
        return _QueryInterface(pThis,pEntries,riid,interface_ptr);
    }

private:
    ref_counter refcount_;
};

typedef ComObjectBase<int32_t> SingleThreadObject;
typedef ComObjectBase<std::atomic<int32_t>> MultiThreadObject;

template<typename cls>
class ComCoClass : public cls{
public:
    ComCoClass(){

    }
    virtual ~ComCoClass(){}
    virtual int32_t AddRef() {
        return cls::InternalAddRef();
    }
    virtual int32_t Release() {
        int32_t ref = cls::InternalRelease();
        if(ref == 0){
            delete this;
            return 0;
        }
        return ref;
    }
    virtual int32_t QueryInterface(const mr::tinycom::TGUID& iid,void **interface_ptr) {
        return _QueryInterface(this,cls::_GetEntries(),iid,interface_ptr);
    }

    static mr::tinycom::IUnknown* Create() {
      auto object = new ComCoClass<cls>();
      mr::tinycom::IUnknown* unknown = nullptr;
      object->QueryInterface(IID_IUnknown,(void**)&unknown);
      return unknown;
    }

};
#define COM_IMPLEMENTER(cls) mr::tinycom::ComCoClass<cls>
#define COM_IMPLEMENTER_CREATOR(cls) (&mr::tinycom::ComCoClass<cls>::Create)

typedef IUnknown* (*FuncComCreate)();
class IComModule{
public:
    virtual const TGUID& uuid() = 0;
    virtual const char* name() = 0;
    virtual const int32_t count() = 0;
    virtual const CLSID& GetClass(int32_t index) = 0;
    virtual const char* GetMetadata(int32_t index) = 0;
    virtual TRESULT CreateInstance(CLSID clsid,void** unknown) = 0;
    virtual TRESULT RegisterObject(CLSID clsid,const char* metadata,FuncComCreate creator) = 0;
};


class ComModule : public IComModule{
public:
    struct Entry{
        TGUID clsid;
        std::string metadata;
        FuncComCreate creator;
    };
    ComModule(const TGUID& module_id,const std::string& name)
        :module_id_(module_id)
        ,name_(name){

    }

    virtual const TGUID& uuid(){
        return module_id_;
    }
    virtual const char* name(){
        return name_.c_str();
    }
    virtual const int32_t count(){
        return entries_.size();
    }
    virtual const TGUID& GetClass(int32_t index){
        const static TGUID null_guid;;
        if(index >= entries_.size())
            return null_guid;
        return entries_[index].clsid;
    }
    virtual const char* GetMetadata(int32_t index){
        if(index >= entries_.size())
            return nullptr;
        return entries_[index].metadata.c_str();
    }
    virtual TRESULT CreateInstance(CLSID clsid,void** unknown){
        *unknown = nullptr;
        for(const auto& item : entries_){
            if(item.clsid == clsid){
                *unknown = item.creator();
                return S_OK;
            }
        }
        return E_NOTIMPL;
    }
    virtual TRESULT RegisterObject(CLSID clsid,const char* metadata,FuncComCreate creator){
        for(const auto& item : entries_){
            if(item.clsid == clsid)
                return E_INVALIDARG;
        }
        entries_.push_back(Entry{clsid,metadata,creator});
        return 0;
    }
private:
    std::vector<Entry> entries_;
    TGUID module_id_;
    std::string name_;
};

template <class T>
class ComPtr {
public:
    ComPtr (T * ptr = nullptr) : p(ptr) {
        if (p)
            p->AddRef();
    }
    ComPtr (const ComPtr & other) : p(other.p) {
        if (p)
            p->AddRef();
    }

    ~ComPtr () {
        if (p)
            p->Release();
        p = nullptr;
    }

    void operator = (T * other) {
        if (p != other)
            ComPtr(other).Swap(*this);
    }
    void operator = (const ComPtr & other) {
        if (p != other.p)
            ComPtr(other).Swap(*this);
    }
    template <typename U>
    void operator = (const ComPtr<U> & other) {
        ComPtr tmp;
        other.QueryInterface(&tmp);
        Swap(tmp);
    }
    template<typename U>
    ComPtr<U> As() const noexcept
    {
        ComPtr<U> tmp;
        if(p){
            p->QueryInterface(__t_uuidof(U),(void**)&tmp.p);
        }
        return tmp;
    }
    template <class Q>
    TRESULT QueryInterface (Q** arg) const {
        if (!p)
            return E_POINTER;
        if (!arg)
            return E_POINTER;
        if (*arg)
            return E_POINTER;

        return p->QueryInterface(__t_uuidof(Q), reinterpret_cast<void**>(arg));
    }
    template <class CLS>
    TRESULT CoCreateInstanceDirectly(){
        ComPtr<IUnknown> unknown;
        unknown.Attach(ComCoClass<CLS>::Create());
        ComPtr tmp;
        TRESULT ret = unknown.QueryInterface(&tmp);
        Swap(tmp);
        return ret;
    }
    bool IsEqualObject (IUnknown * other) const {
        ComPtr<IUnknown> this_obj;
        this_obj = *this;
        return this_obj == other;
    }

    TRESULT CopyTo (T** arg) {
        if (!arg)
            return E_POINTER;
        if (*arg)
            return E_POINTER; // input must be pointer to nullptr

        *arg = p;
        if (p)
            p->AddRef();

        return S_OK;
    }

    /** Take over ownership (does not incr. ref-count). */
    void Attach (T * ptr) {
        if (p)
            p->Release();
        p = ptr;
        // no AddRef
    }
    /** Release ownership (does not decr. ref-count). */
    T* Detach () {
        T * tmp = p;
        p = nullptr;
        // no Release
        return tmp;
    }

    T* Get () {
        return p;
    }
    void Release () {
        if (!p)
            return;

        p->Release();
        p = nullptr;
    }

    operator T*() const {
        return p;
    }
    T* operator -> () const {
        assert(p && "CComPtr::operator -> nullptr.");
        return p;
    }
    T** operator & () {
        return &p;
    }

    T * p = nullptr;

protected:
    void Swap (ComPtr & other) {
        T* tmp = p;
        p = other.p;
        other.p = tmp;
    }
};

typedef mr::tinycom::ComPtr<mr::tinycom::IUnknown> IUnknownPtr;


class ComRepository{
public:
    int32_t RegisterModule(IComModule* module){
        if(!module)
            return E_POINTER;
        int32_t index = 0;
        CLSID clsid;
        do{
            clsid = module->GetClass(index);
            if(clsid.empty())
                break;
            creators_[clsid] = module;
            metadates_[clsid] = module->GetMetadata(index);

            ++index;
        }while (true);

        return 0;
    }
    IUnknownPtr CreateObject(const CLSID& clsid){
        IUnknownPtr unknown_ptr;
        if(creators_.find(clsid) == creators_.end())
            return unknown_ptr;
        IUnknown* unknown = nullptr;
        creators_[clsid]->CreateInstance(clsid,(void**)&unknown);
        unknown_ptr.Attach(unknown);
        return unknown_ptr;
    }
    const char* metadata(const CLSID& clsid){
        if(metadates_.find(clsid) == metadates_.end())
            return nullptr;
        return metadates_[clsid].c_str();
    }
private:
    std::vector<IComModule*> modules_;
    std::map<CLSID,IComModule*> creators_;
    std::map<CLSID,std::string> metadates_;
};

#if defined(_MSC_VER)
#define DECLARE_NOVTABLE __declspec(novtable)

#else
#define DECLARE_NOVTABLE

#endif

/*
com interface define macros
*/
#define COM_INTERFACE(iid,name)                                                                     \
static const char* const IID_##name = iid;                                                          \
class DECLARE_NOVTABLE name : public mr::tinycom::IUnknown {                                    \
public:                                                                                             \
  virtual ~name(){};                                                                                \
  static const mr::tinycom::TGUID& uuid() {                                                             \
    static mr::tinycom::TGUID iid_(IID_##name);                                                         \
    return iid_;                                                                                    \
  }

#define COM_METHOD(_name_arg) virtual TRESULT _name_arg = 0;
#define COM_METHOD_RET(_type, _name_arg) virtual _type _name_arg = 0;

#define COM_IMP_METHOD(_name_arg) virtual TRESULT _name_arg;
#define COM_IMP_METHOD_RET(_type, _name_arg) virtual _type _name_arg;
/*
com object class define macros
*/
#define COM_OBJECT(thread_mod,clsid,metadata,CLS)                                                  \
    static const char* const CLSID_##CLS = clsid;                                                  \
    static const char* const METADATA_##CLS = metadata;                                            \
    class DECLARE_NOVTABLE CLS                                                             \
        : public thread_mod

#define COM_SINGLEHREADED_OBJECT(clsid,metadata,name)  COM_OBJECT(mr::tinycom::SingleThreadObject,clsid,metadata,name)
#define COM_MULTITHREADED_OBJECT(clsid,metadata,name)  COM_OBJECT(mr::tinycom::MultiThreadObject,clsid,metadata,name)

#define COM_IMPLEMENT_INTERFACE(name) , public name

/*
com object-interfaces mapping define macros
*/
#define COM_OFFSET_OF(base, i) ((std::ptrdiff_t)(static_cast<base*>((i*)0x08)) - 0x08)

#define COM_MAP_BEGINE(CLS)                                                                         \
    public:                                                                                         \
    typedef CLS _ThisObjectType;                                                                    \
    const static mr::tinycom::TGUID& uuid() {                                                           \
      static mr::tinycom::TGUID clsid_(CLSID_##CLS);                                                    \
      return clsid_;                                                                                \
    };                                                                                              \
    static const char* metadata() {                                                                 \
      static std::string metadata_(METADATA_##CLS);                                                 \
        return metadata_.c_str();                                                                   \
    };                                                                                              \
    const static mr::tinycom::InterfaceEntry* _GetEntries() {                                           \
      static const mr::tinycom::InterfaceEntry _entries[] = {

#define COM_INTERFACE_ENTRY(i) {IID_##i, COM_OFFSET_OF(i, _ThisObjectType)},

#define COM_MAP_END()                                                                               \
        { mr::tinycom::TGUID(), INTERFACE_INVALID_OFFSET }                                              \
    };                                                                                              \
    return _entries;                                                                                \
}

#define COM_FAKER_UNKNOWN \
virtual int32_t AddRef() override {return 0;}\
virtual int32_t Release() override {return 0;}\
virtual int32_t QueryInterface(mr::tinycom::REFIID iid, void **interface) override {*interface=nullptr;return 0;}


#define AS_STR(S) #S
/*
COM OBJECT REGISTER MACROS
*/
#define COM_REGISTER_OBJECT(CLS) \
    const mr::tinycom::TGUID& g_t_uuidof_##CLS(){return __t_uuidof(CLS);}\
    const mr::tinycom::FuncComCreate g_creatorof_##CLS(){return COM_IMPLEMENTER_CREATOR(CLS);}\
    const char* g_metadataof_##CLS(){return __t_metadataof(CLS);}

#define COM_OBJECT_ENTRY_IMPORT(CLS) \
    extern const mr::tinycom::TGUID& g_t_uuidof_##CLS();\
    extern const mr::tinycom::FuncComCreate g_creatorof_##CLS();\
    extern const char* g_metadataof_##CLS();

#define COM_OBJECT_ENTRY_UUID(CLS) g_t_uuidof_##CLS()
#define COM_OBJECT_ENTRY_CREATOR(CLS) g_creatorof_##CLS()
#define COM_OBJECT_ENTRY_METADATA(CLS) g_metadataof_##CLS()

/*
COM MODULE REGISTER MACROS
*/
#define COM_MODULE_BEGINE(ID,MODULENAME) \
mr::tinycom::IComModule* g_com_get_module_##MODULENAME(){\
    static mr::tinycom::ComModule s_com_module_(ID,AS_STR(MODULENAME));\
    if(s_com_module_.count() == 0){

#define COM_MODULE_OBJECT_ENTRY(CLS)\
    COM_OBJECT_ENTRY_IMPORT(CLS)\
    s_com_module_.RegisterObject(COM_OBJECT_ENTRY_UUID(CLS),COM_OBJECT_ENTRY_METADATA(CLS),COM_OBJECT_ENTRY_CREATOR(CLS));\

#define COM_MODULE_END() \
    }\
    return static_cast<mr::tinycom::IComModule*>(&s_com_module_);\
}

#define COM_MODULE_IMPORT(MODULENAME) extern mr::tinycom::IComModule* g_com_get_module_##MODULENAME();
#define COM_MODULE_GET(MODULENAME) g_com_get_module_##MODULENAME()

}


#endif // mr::tinycom_H
