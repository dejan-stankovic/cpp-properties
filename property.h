#include <functional>
#include <unordered_map>

// Base class of TypedProperty; object-specific but not value type-specific
template<typename Object> class Property
{
public:
    const char * const name;
    const bool writable;

    Property(const char *name, bool writable)
        : name(name)
        , writable(writable)
    {
    }
    virtual ~Property() { }

    template<typename T> T get(Object *object);
    template<typename T> void set(Object *object, T value);
};

template<typename T, typename O> class TypedProperty : public Property<O>
{
public:
    const std::function<T(O*)> get;
    const std::function<void(O*,T)> set;

    TypedProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set)
        : Property<O>(name, (bool)set)
        , get(get)
        , set(set)
    {
    }
};

// XXX abort() is not a great reaction to mismatched types
template<typename Object> template<typename T> T Property<Object>::get(Object *object)
{
    auto val = dynamic_cast<TypedProperty<T,Object>*>(this);
    if (!val)
        abort();
    return val->get(object);
}

template<typename Object> template<typename T> void Property<Object>::set(Object *object, T value)
{
    auto val = dynamic_cast<TypedProperty<T,Object>*>(this);
    if (!val)
        abort();
    if (!writable)
        abort();
    val->set(object, value);
}

// Properties holds a map of properties for a type. Usually declared as a static member and
// defined like:
//
// Properties<Thing> Thing::properties = {
//     MakeProperty("message", &Thing::getMessage, &Thing::setMessage),
//     MakeProperty("readonly", &Thing::getMessage)
// };
//
template<typename Object> class Properties
{
public:
    Properties(std::initializer_list<Property<Object>*> props)
    {
        for (auto prop : props)
            properties.insert({prop->name, prop});
    }
    Properties(const Properties &) = delete;
    Properties &operator=(const Properties &) = delete;

    template<typename T> T get(Object *object, const char *prop) const
    {
        return properties.at(prop)->template get<T>(object);
    }

    template<typename T> void set(Object *object, const char *prop, T value)
    {
        return properties.at(prop)->template set<T>(object, value);
    }

    Property<Object> *operator[](const char *prop)
    {
        return properties.at(prop);
    }

private:
    std::unordered_map<const char*, Property<Object>*> properties;
};

// An empty set function makes a read-only property
template<typename T, typename O> Property<O> *MakeProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set = nullptr)
{
    // XXX leaked.. would be nice if this was allocated differently :shrug:
    return new TypedProperty<T,O>{name, get, set};
}

// Overload for template deduction; allows the use of: MakeProperty(&Object::get, &Object::set)
template<typename T, typename O> Property<O> *MakeProperty(const char name[], T(O::*get)(), void(O::*set)(T) = nullptr)
{
    return MakeProperty<T,O>(name, std::function<T(O*)>(get), std::function<void(O*,T)>(set));
}


