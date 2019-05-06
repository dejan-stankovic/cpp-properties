#include <functional>
#include <unordered_map>

// Silly little stand-in for a variant type, don't read into it
struct Value
{
    union {
        int intValue;
        const char *charValue;
    };
    bool isInt;

    Value(int v) : intValue(v), isInt(true) { }
    Value(const char *v) : charValue(v), isInt(false) { }

    template<typename T> T convert();

    void print() {
        if (isInt)
            printf("value int: %d\n", intValue);
        else
            printf("value string: %s\n", charValue);
    }
};

template<> int Value::convert<int>()
{
    if (isInt)
        return intValue;
    else
        return 0;
};

template<> const char *Value::convert<const char*>()
{
    if (isInt)
        return nullptr;
    else
        return charValue;
};

class Object;

// Generic base class of TypedProperty and all properties
class Property
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

    // Returns true if the object is compatible with the object type of this property
    virtual bool isOfObject(Object *object) = 0;

    // Gets/sets the property with a generic Value
    virtual Value getValue(Object *object) = 0;
    virtual void setValue(Object *object, Value value) = 0;

    template<typename T> T get(Object *object);
    template<typename T> void set(Object *object, T value);
};

// TypedProperty is the type-specific subclass of Property, handling the getter/setter functions.
// It is not object type specific.
template<typename T> class TypedProperty : public Property
{
public:
    std::function<T(Object*)> get;
    std::function<void(Object*,T)> set;

    TypedProperty(const char name[], std::function<T(Object*)> get, std::function<void(Object*,T)> set)
        : Property(name, (bool)set)
        , get(get)
        , set(set)
    {
    }

    virtual Value getValue(Object *object) override
    {
        return get(object);
    }

    virtual void setValue(Object *object, Value value) override
    {
        return set(object, value.convert<T>());
    }
};

// ObjectTypedProperty is unique to an object type and value type, capable of safely
// casting anything. There may be more direct ways to implement some of this.
template<typename T, typename O> class ObjectTypedProperty : public TypedProperty<T>
{
public:
    ObjectTypedProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set)
        : TypedProperty<T>(name,
                           [get](Object *o) { return get(static_cast<O*>(o)); },
                           [set](Object *o, T value) { set(static_cast<O*>(o), value); })
    {
    }

    virtual bool isOfObject(Object *object) override
    {
        return dynamic_cast<O*>(object) != nullptr;
    }
};

// Properties holds a map of properties for a type. Usually declared as a static member and
// defined like:
//
// Properties<Thing> Thing::properties = {
//     MakeProperty("message", &Thing::getMessage, &Thing::setMessage),
//     MakeProperty("readonly", &Thing::getMessage)
// };
//
class Properties
{
public:
    Properties(std::initializer_list<Property*> props)
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

    Property *operator[](const char *prop)
    {
        return properties.at(prop);
    }

private:
    std::unordered_map<const char*, Property*> properties;
};

// Object is a base class, provided for easy casting. It isn't otherwise meaningful.
// Properties is statically allocated in the subclass, so it's awkwardly passed in here.
class Object
{
public:
    Object(Properties &props)
        : m_properties(props)
    {
    }
    virtual ~Object() { }

    Property *property(const char *prop) { return m_properties[prop]; }
    template<typename T> T getProperty(const char *prop) { return m_properties.get<T>(this, prop); }
    template<typename T> void setProperty(const char *prop, T value) { m_properties.set<T>(this, prop, value); }

private:
    Properties &m_properties;
};

// Specialize getProperty/setProperty for Value
template<> Value Object::getProperty<Value>(const char *name)
{
    auto prop = m_properties[name];
    return prop->getValue(this);
}

template<> void Object::setProperty<Value>(const char *name, Value value)
{
    auto prop = m_properties[name];
    prop->setValue(this, value);
}

// XXX abort() is not a great reaction to mismatched types
template<typename T> T Property::get(Object *object)
{
    auto val = dynamic_cast<TypedProperty<T>*>(this);
    if (!val)
        abort();
    return val->get(object);
}

template<typename T> void Property::set(Object *object, T value)
{
    auto val = dynamic_cast<TypedProperty<T>*>(this);
    if (!val)
        abort();
    if (!writable)
        abort();
    val->set(object, value);
}

// An empty set function makes a read-only property
template<typename T, typename O> Property *MakeProperty(const char name[], std::function<T(O*)> get, std::function<void(O*,T)> set = nullptr)
{
    // XXX leaked.. would be nice if this was allocated differently :shrug:
    return new ObjectTypedProperty<T,O>{name, get, set};
}

// Overload for template deduction; allows the use of: MakeProperty(&Object::get, &Object::set)
template<typename T, typename O> Property *MakeProperty(const char name[], T(O::*get)(), void(O::*set)(T) = nullptr)
{
    return MakeProperty<T,O>(name, std::function<T(O*)>(get), std::function<void(O*,T)>(set));
}

