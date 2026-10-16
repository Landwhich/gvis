#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <vulkan_shared.h>

namespace gv = GVIS;

class Resource{
  protected:
    std::string resourceID;
    bool        loaded = false;
  public:
    explicit Resource(const std::string& id) : resourceID(id) {};
    virtual ~Resource() = default;

    const std::string& GetID() const { return this->resourceID; }
    bool IsLoaded() const { return this->loaded; }
    // public universal interface with a virtual backend
    bool Load(const std::pair<gv::DeviceContext, gv::GraphicsContext>& resourceContext){
        loaded = doLoad(resourceContext);
        return loaded;
    }

    void Unload(){
        doUnload();
        loaded = false;
    }
  protected:
    virtual bool doLoad(
        const std::pair<gv::DeviceContext, gv::GraphicsContext>& resourceContext) = 0;
    virtual void doUnload() = 0;
};

template<typename T>
class ResourceHandle{
    std::shared_ptr<T> resource;
  public:
    ResourceHandle() = default;
    ResourceHandle(std::shared_ptr<T> r) : resource(std::move(r)) {};

    T* Get() const { return resource.get(); }
    bool IsValid() const { return resource && resource->IsLoaded(); }
    const std::string& GetID() const {
        static const std::string empty;
        return resource ? resource->GetID() : empty; 
    }
    // operator overloads (for convenience)
    T* operator->() const { return resource.Get(); }
    T& operator*() const { return *resource; }
    explicit operator bool() const { return IsValid(); }
};

class ResourceManager {
    // stores by type -> then by id 
    std::unordered_map<std::type_index, std::unordered_map
        <std::string, std::shared_ptr<Resource>>> resources;

    const std::pair<gv::DeviceContext, gv::GraphicsContext> resourceContext;

  public:    
    explicit ResourceManager(std::pair<gv::DeviceContext, gv::GraphicsContext> ctx) 
       : resourceContext(ctx) {}
    
template <typename T, typename... Args>
ResourceHandle<T> LoadResource(const std::string& id, Args&& ...args){ 
    static_assert(std::is_base_of<Resource, T>::value, "Template argument must derive from Resource");

    auto &typeResources = resources[std::type_index(typeid(T))];
    auto  it            = typeResources.find(id);
    if (it != typeResources.end())
        return ResourceHandle<T>(std::static_pointer_cast<T>(it->second));

    // The deleter calls Unload() before delete so it still goes through T's vtable.
    std::shared_ptr<T> resource(new T(id, std::forward<Args>(args)...), [](T *r) {
        r->Unload();
        delete r;
    });
    if (!resource->Load(this->resourceContext))
        throw std::runtime_error("Failed to load resource: " + id);

    typeResources[id] = resource;
    return ResourceHandle<T>(resource);
}

template <typename T>
ResourceHandle<T> GetResource(const std::string& id) {
    static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

    auto typeIt = resources.find(std::type_index(typeid(T)));
    if (typeIt == resources.end())
        return ResourceHandle<T>();

    auto &typeResources = typeIt->second;
    auto  resourceIt    = typeResources.find(id);
    if (resourceIt == typeResources.end())
        return ResourceHandle<T>();

    return ResourceHandle<T>(std::static_pointer_cast<T>(resourceIt->second));
}

template <typename T>
long UseCount(const std::string& id) const {
    static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

    auto typeIt = resources.find(std::type_index(typeid(T)));
    if (typeIt == resources.end())
        return 0;

    auto &typeResources = typeIt->second;
    auto  resourceIt    = typeResources.find(id);
    if (resourceIt == typeResources.end())
        return 0;

    return resourceIt->second.use_count();
}

template <typename T>
bool HasResource(const std::string& id) const {
    static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

    auto typeIt = resources.find(std::type_index(typeid(T)));
    if (typeIt == resources.end())
        return false;

    auto &typeResources = typeIt->second;
    return typeResources.contains(id);
}

template <typename T>
bool UnloadResource(const std::string& id) {
    static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

    auto typeIt = resources.find(std::type_index(typeid(T)));
    if (typeIt == resources.end())
        return false;

    auto &typeResources = typeIt->second;
    auto  resourceIt    = typeResources.find(id);
    if (resourceIt == typeResources.end())
        return false;

    typeResources.erase(resourceIt);
    return true;
}

    void UnloadAll() { resources.clear(); }

};

