#include "property.h"

class Thing
{
public:
    static Properties<Thing> properties;

    const char *getMessage() { return m_message; }
    void setMessage(const char *value) { m_message = value; }

    int unchanged() { return 4; }

private:
    const char *m_message = "hello world";
};

Properties<Thing> Thing::properties = {
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
    return 0;
}
