#ifndef PERSON_H
#define PERSON_H

#include <string>

class Person {
protected:
    std::string name;
    int id;
public:
    Person(std::string name, int id): name(name), id(id) {}
    virtual ~Person() = default;
    std::string getName() const { return name; }
    int getId() const { return id; }
};

class User : public Person {
public:
    User(std::string name, int id): Person(name,id) {}
};

class Driver : public Person {
    bool available;
public:
    Driver(std::string name, int id): Person(name,id), available(true) {}
    bool isAvailable() const { return available; }
    void setAvailable(bool a) { available = a; }
};

#endif
