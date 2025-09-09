#include "Dispatcher.h"
#include "Payment.h"

int main() {
    Dispatcher dsp;
    dsp.addUser(User("Alice",1));
    dsp.addUser(User("Bob",2));
    dsp.addDriver(Driver("Charlie",101));
    dsp.addDriver(Driver("David",102));

    auto ride = dsp.bookRide(1, Coord(0,0), Coord(3,4), new StandardFare());
    if(ride) {
        ride->showDetails();
        auto pay = PaymentFactory::getProcessor("card");
        pay->pay(120.5);
        ride->complete();
    }

    return 0;
}
