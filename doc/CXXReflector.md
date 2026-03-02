# Cxx Reflector

```cpp
KXX(Serialize)
struct Request {
    std::string name;
    std::string func;
};


KXX(Serialize)
struct Response {
    std::string data;

    // reflector should check whether has implementation
    virtual void serialize();
};

// generated function is serialize_{namespaceA_namespaceB}_Response(Response& rsp);
//                      or namespaceA::namespaceB::serialize(Response& rsp);

// Meta:
//  Type
//      map<string: name, type_prop>        
//  Struct
//      map<string: fieldname, string: type>
//  Enum
//  Function


KXX(Rpc)
struct IService {
    virtual ~IService() {}

    virtual void put(const Request& req) = 0;
    virtual Response getResp() = 0;
};

// Server code
struct ServiceHandler : public IService {
public:

    void put(const Request& req) override
    {
        
    }

    Response getResp() override 
    {
        
    }

    void dispatch()
    {   // codegen
        recv(method);
        switch (method)
        {
            case "put":
            {
                recv(req);
                put(req);
                send(none)
                break;
            }
            case "getResp":
            {
                recv(none)
                rsp = getResp();
                send(rsp);
                break;
            }
        }
    }
};


struct Client : public IService
{
    void put(const Request& req) override
    {   // codegen
        send("put", serialize(req));
    }

    Response getResp() override 
    {   // codegen
        send("getResp", none);
        Response rsp;
        recv(rsp)
        return rsp;
    }
};

```