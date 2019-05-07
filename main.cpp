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
    t.properties.get(&t, "message").print();
    auto prop = t.properties["message"];
    prop->set(&t, "hello universe");
    prop->get(&t).print();

    // this one is an int
    t.getProperty("unchanged").print();

    // This will set to null, because that's how Value treats type mismatch.
    Value v(5);
    t.setProperty("message", v);
    t.getProperty("message").print();

    return 0;
}
