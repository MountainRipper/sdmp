
#include "../sdmp/interface/tinycom.h"

#if defined(_MSVC)
#define __FUNCTION_NAME__ __FUNCTION__
#else
#define __FUNCTION_NAME__ __PRETTY_FUNCTION__
#endif

COM_INTERFACE("f37a7b18-a06e-11ed-b21f-dbca22e98ba7",IFruit)
    COM_METHOD(count(int size))
    COM_METHOD_RET(const char*,name())
};

COM_INTERFACE("a085b87e-a0b3-11ed-8bdc-a3bc8aa7edc1",IRed)
    COM_METHOD(red())
};

COM_INTERFACE("981653ba-a0b3-11ed-906b-73263e66853a",IYellow)
    COM_METHOD(yellow())
};

COM_MULTITHREADED_OBJECT("1ca517de-a077-11ed-b63b-fb310a574611","Apple meta data",Apple)
COM_IMPLEMENT_INTERFACE(IFruit)
COM_IMPLEMENT_INTERFACE(IRed)
{

    COM_MAP_BEGINE(Apple)
            COM_INTERFACE_ENTRY(IFruit)
            COM_INTERFACE_ENTRY(IRed)
    COM_MAP_END()

    Apple(){
        fprintf(stderr,"%s constructor\n",__FUNCTION_NAME__);
    }
    ~Apple(){
        fprintf(stderr,"%s destructor\n",__FUNCTION_NAME__);
    }
    HRESULT count(int size){
        fprintf(stderr,"%s size:%d\n",__FUNCTION_NAME__,size);
        return size * 2;
    }
    const char* name(){
        fprintf(stderr,"%s name:apple\n",__FUNCTION_NAME__);
        return "apple";
    }

    HRESULT red(){
        fprintf(stderr,"%s red:apple is red\n",__FUNCTION_NAME__);
        return 0;
    }
};
COM_REGISTER_OBJECT(Apple)

COM_MULTITHREADED_OBJECT("e2a217de-a0b3-11ed-81f5-9b643495803a","Pear meta data",Pear)
COM_IMPLEMENT_INTERFACE(IFruit)
COM_IMPLEMENT_INTERFACE(IYellow)
{

    COM_MAP_BEGINE(Pear)
        COM_INTERFACE_ENTRY(IFruit)
        COM_INTERFACE_ENTRY(IYellow)
    COM_MAP_END()

    Pear(){
        fprintf(stderr,"%s constructor\n",__FUNCTION_NAME__);
    }
    ~Pear(){
        fprintf(stderr,"%s destructor\n",__FUNCTION_NAME__);
    }
    HRESULT count(int size){
        fprintf(stderr,"%s size:%d\n",__FUNCTION_NAME__,size);
        return size * 2;
    }
    const char* name(){
        fprintf(stderr,"%s name: pear\n",__FUNCTION_NAME__);
        return "apple";
    }
    HRESULT yellow(){
        fprintf(stderr,"%s yellow:pear is yellow\n",__FUNCTION_NAME__);
        return 0;
    }
};
COM_REGISTER_OBJECT(Pear)

int main(){
    //tinycom::ComFactory::factory()->RegisterObject(__uuidof(Apple),&tinycom::CComCoClass<Apple>::Create);
    tinycom::IUnknownPtr i1;
    i1.CoCreateInstanceDirectly<Apple>();

    tinycom::ComPtr<IFruit> f1;
    i1->QueryInterface(__t_uuidof(IFruit),(void**)&f1);
    f1->name();
    f1->count(2);

    tinycom::ComPtr<IRed> r;
    f1->QueryInterface(__t_uuidof(IRed),(void**)&r);
    r->red();


    tinycom::IUnknownPtr i2;
    i2.CoCreateInstanceDirectly<Pear>();

    IFruit* f2 = nullptr;
    i2.QueryInterface(&f2);
    f2->name();
    f2->count(20);

    tinycom::ComPtr<IYellow> y;
    f2->QueryInterface(__t_uuidof(IYellow),(void**)&y);
    y->yellow();
    f2->Release();

    return 0;
}
