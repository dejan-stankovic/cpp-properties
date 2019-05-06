#include "property.h"

class Thing : public Object
{
public:
    static Properties properties;

    Thing() : Object(properties) { }

    const char *getMessage() { return m_message; }
    void setMessage(const char *value) { m_message = value; }

    int unchanged() { return 4; }

private:
    const char *m_message = "hello world";
};

Properties Thing::properties = {
    MakeProperty("message", &Thing::getMessage, &Thing::setMessage),
    MakeProperty("unchanged", &Thing::unchanged)
};

int main(int argc, char **argv)
{
    Thing t;
    printf("get: \"%s\"\n", t.properties.get<const char*>(&t, "message"));
    auto prop = t.properties["message"];
    prop->set(&t, "hello universe");
    printf("get2: \"%s\"\n", prop->get<const char*>(&t));
    printf("get3: \"%d\"\n", t.properties.get<int>(&t, "unchanged"));

    Value v = t.getProperty<Value>("message");
    v.print();

    v = Value("hello value");
    t.setProperty("message", v);
    t.getProperty<Value>("message").print();

    v = Value(5);
    t.setProperty("message", v);
    t.getProperty<Value>("message").print();

    return 0;
}
