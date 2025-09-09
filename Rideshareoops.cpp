#include <bits/stdc++.h>
using namespace std;

// -------------- Utilities --------------
struct Coord {
    double x{0}, y{0};
};
static double distanceKm(const Coord&a, const Coord&b){
    double dx=a.x-b.x, dy=a.y-b.y; return sqrt(dx*dx+dy*dy); // Euclidean for demo
}

// -------------- Exceptions --------------
struct AppError : runtime_error { using runtime_error::runtime_error; };
struct NotFound : AppError { using AppError::AppError; };
struct InvalidState : AppError { using AppError::AppError; };

// -------------- Observer Pattern --------------
struct IObserver { virtual ~IObserver()=default; virtual void onNotify(const string&msg)=0; };
struct ISubject {
    virtual ~ISubject()=default;
    virtual void subscribe(IObserver*)=0;
    virtual void unsubscribe(IObserver*)=0;
    virtual void notifyAll(const string&msg)=0;
};

// -------------- Base Entities --------------
class Person : public IObserver {
protected:
    int id; string name; double ratingSum{0}; int ratingCount{0};
public:
    Person(int id,string name):id(id),name(std::move(name)){}
    virtual ~Person() = default;
    int getId() const { return id; }
    const string& getName() const { return name; }
    double rating() const { return ratingCount? ratingSum/ratingCount : 0.0; }
    void addRating(int r){ ratingSum += r; ratingCount++; }
    void onNotify(const string&msg) override { cout << "[Notify:"<< name << "] " << msg << '\n'; }
};

class User : public Person {
    Coord location; // current location
public:
    User(int id, string name, Coord loc):Person(id, std::move(name)), location(loc){}
    const Coord& getLocation() const { return location; }
    void setLocation(Coord c){ location=c; }
};

class Driver : public Person {
    string carModel; string plate; bool available{true}; Coord location;
public:
    Driver(int id,string name,string car,string plate, Coord loc)
        :Person(id,std::move(name)),carModel(std::move(car)),plate(std::move(plate)),location(loc){}
    bool isAvailable() const { return available; }
    void setAvailable(bool v){ available=v; }
    const Coord& getLocation() const { return location; }
    void setLocation(Coord c){ location=c; }
    const string& getCar() const { return carModel; }
    const string& getPlate() const { return plate; }
};

// -------------- Fare Strategy --------------
struct RideContext; // fwd
struct IFareStrategy { virtual ~IFareStrategy()=default; virtual double calculate(const RideContext&) const =0; virtual string name() const =0; };

struct StandardFare : IFareStrategy { // base fare + per-km + per-min
    double base{30}, perKm{12}, perMin{2};
    double calculate(const RideContext&ctx) const override;
    string name() const override { return "Standard"; }
};
struct SurgeFare : IFareStrategy { // multiplies standard by surgeFactor
    double surge{1.5}; StandardFare std;
    double calculate(const RideContext&ctx) const override { return std.calculate(ctx) * surge; }
    string name() const override { return string("Surge x") + to_string(surge); }
};
struct PoolFare : IFareStrategy { // cheaper per-km, but small wait charge
    double base{20}, perKm{8}, perMin{1.5};
    double calculate(const RideContext&ctx) const override;
    string name() const override { return "Pool"; }
};

// -------------- Payment Factory --------------
struct IPaymentProcessor { virtual ~IPaymentProcessor()=default; virtual bool pay(double amount)=0; virtual string method() const =0; };
struct UpiPayment : IPaymentProcessor { string upiId; explicit UpiPayment(string id):upiId(std::move(id)){} bool pay(double amt) override { cout<<"[UPI] Paying Rs."<<amt<<" via "<<upiId<<"... OK\n"; return true; } string method() const override { return "UPI"; } };
struct CardPayment : IPaymentProcessor { string last4; explicit CardPayment(string l4):last4(std::move(l4)){} bool pay(double amt) override { cout<<"[CARD] Paying Rs."<<amt<<" with ****"<<last4<<"... OK\n"; return true; } string method() const override { return "CARD"; } };
struct CashPayment : IPaymentProcessor { bool pay(double amt) override { cout<<"[CASH] Collect Rs."<<amt<<" in cash.\n"; return true; } string method() const override { return "CASH"; } };

struct PaymentFactory {
    static unique_ptr<IPaymentProcessor> create(const string&kind){
        if(kind=="upi") return make_unique<UpiPayment>("user@upi");
        if(kind=="card") return make_unique<CardPayment>("1234");
        if(kind=="cash") return make_unique<CashPayment>();
        throw AppError("Unknown payment kind: "+kind);
    }
};

// -------------- Repository (Template) --------------
template<class T>
class Repository {
    unordered_map<int, shared_ptr<T>> data;
public:
    void add(shared_ptr<T> obj){ data[obj->getId()] = std::move(obj); }
    shared_ptr<T> get(int id) const {
        auto it = data.find(id);
        if(it==data.end()) throw NotFound("ID not found");
        return it->second;
    }
    vector<shared_ptr<T>> all() const { vector<shared_ptr<T>> v; v.reserve(data.size()); for(auto &p:data) v.push_back(p.second); return v; }
};

// -------------- Ride --------------
enum class RideStatus { Requested, Accepted, OnTrip, Completed, Cancelled };
static string to_string(RideStatus s){
    switch(s){
        case RideStatus::Requested: return "Requested";
        case RideStatus::Accepted: return "Accepted";
        case RideStatus::OnTrip: return "OnTrip";
        case RideStatus::Completed: return "Completed";
        case RideStatus::Cancelled: return "Cancelled";
    }
    return "?";
}

struct RideContext { double km{0}; double minutes{0}; };

auto nowStr = [](){
    time_t t=time(nullptr); char buf[64]; strftime(buf,sizeof(buf),"%F %T", localtime(&t)); return string(buf);
};

class Ride : public ISubject {
    int id; shared_ptr<User> rider; shared_ptr<Driver> driver; Coord from{}, to{}; RideStatus status{RideStatus::Requested};
    unique_ptr<IFareStrategy> fareStrategy; double fare{0}; RideContext ctx; vector<IObserver*> observers;
public:
    Ride(int id, shared_ptr<User> u, shared_ptr<Driver> d, Coord from, Coord to, unique_ptr<IFareStrategy> fs)
        :id(id), rider(std::move(u)), driver(std::move(d)), from(from), to(to), fareStrategy(std::move(fs)){
            ctx.km = distanceKm(from,to);
            ctx.minutes = max(8.0, ctx.km*3.0); // toy estimate
        }
    int getId() const { return id; }
    shared_ptr<User> getUser() const { return rider; }
    shared_ptr<Driver> getDriver() const { return driver; }
    RideStatus getStatus() const { return status; }
    double getFare() const { return fare; }
    const RideContext& getContext() const { return ctx; }

    void subscribe(IObserver*o) override { observers.push_back(o); }
    void unsubscribe(IObserver*o) override { observers.erase(remove(observers.begin(),observers.end(),o), observers.end()); }
    void notifyAll(const string&msg) override { for(auto*o:observers) o->onNotify(msg); }

    void accept(){ if(status!=RideStatus::Requested) throw InvalidState("Cannot accept now"); status=RideStatus::Accepted; driver->setAvailable(false); notifyAll("Ride " + to_string(id) + " accepted at "+ nowStr()); }
    void start(){ if(status!=RideStatus::Accepted) throw InvalidState("Cannot start now"); status=RideStatus::OnTrip; notifyAll("Ride " + to_string(id) + " started at "+ nowStr()); }
    void complete(){ if(status!=RideStatus::OnTrip) throw InvalidState("Cannot complete now"); fare = fareStrategy->calculate(ctx); status=RideStatus::Completed; driver->setAvailable(true); notifyAll("Ride " + to_string(id) + " completed. Fare: Rs." + to_string(fare)); }
    void cancel(){ if(status==RideStatus::Completed) throw InvalidState("Already completed"); status=RideStatus::Cancelled; driver->setAvailable(true); notifyAll("Ride " + to_string(id) + " cancelled."); }
    string fareName() const { return fareStrategy? fareStrategy->name():"?"; }
};

// Fare formula implementations (after RideContext available)
double StandardFare::calculate(const RideContext&ctx) const { return base + ctx.km*perKm + ctx.minutes*perMin; }
double PoolFare::calculate(const RideContext&ctx) const { return base + ctx.km*perKm + ctx.minutes*perMin + 5; }

// -------------- Matching / Dispatch --------------
class Dispatcher {
    Repository<User> &users; Repository<Driver> &drivers; int nextRideId{1000};
public:
    explicit Dispatcher(Repository<User>&u, Repository<Driver>&d):users(u),drivers(d){}

    shared_ptr<Driver> findNearestDriver(const Coord&src){
        shared_ptr<Driver> best=nullptr; double bestDist=1e18;
        for(auto &d: drivers.all()){
            if(!d->isAvailable()) continue;
            double dist = distanceKm(d->getLocation(), src);
            if(dist < bestDist){ bestDist=dist; best=d; }
        }
        if(!best) throw NotFound("No drivers available");
        return best;
    }

    unique_ptr<IFareStrategy> makeStrategy(const string&type){
        if(type=="standard") return make_unique<StandardFare>();
        if(type=="surge") return make_unique<SurgeFare>();
        if(type=="pool") return make_unique<PoolFare>();
        throw AppError("Unknown ride type: "+type);
    }

    shared_ptr<Ride> requestRide(int userId, Coord from, Coord to, const string&fareType){
        auto u = users.get(userId);
        auto d = findNearestDriver(from);
        auto strategy = makeStrategy(fareType);
        auto ride = make_shared<Ride>(nextRideId++, u, d, from, to, std::move(strategy));
        ride->subscribe(u.get());
        ride->subscribe(d.get());
        cout << "Assigned Driver: "<< d->getName() << " ("<< d->getCar() << ", "<< d->getPlate() << ")\n";
        return ride;
    }
};

// -------------- Demo CLI --------------
static void printDriver(const shared_ptr<Driver>&d){
    cout<<"#"<<d->getId()<<" "<<left<<setw(12)<<d->getName()<<" | Car: "<<setw(10)<<d->getCar()<<" | Plate: "<<setw(8)<<d->getPlate()
        <<" | Avail: "<<(d->isAvailable()?"Yes":"No")<<" | Rating: "<<fixed<<setprecision(2)<<d->rating()<<"\n";
}

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);

    Repository<User> users; Repository<Driver> drivers;
    // Seed data
    users.add(make_shared<User>(1, "Aarav", Coord{0,0}));
    users.add(make_shared<User>(2, "Ishita", Coord{5,3}));

    drivers.add(make_shared<Driver>(101, "Ramesh", "Swift", "DL1AA1111", Coord{1,1}));
    drivers.add(make_shared<Driver>(102, "Suman",  "i20",   "HR26BB2222", Coord{6,4}));
    drivers.add(make_shared<Driver>(103, "Faizan", "WagonR","UP14CC3333", Coord{15,2}));

    Dispatcher dispatch(users, drivers);

    vector<shared_ptr<Ride>> rides;

    cout << "\n===== Ride-Sharing (C++ OOP Advanced) =====\n";
    cout << "Commands:\n"
            " 1) list-drivers\n"
            " 2) request-ride <userId> <fromX> <fromY> <toX> <toY> <type: standard|surge|pool>\n"
            " 3) advance <rideId> <action: accept|start|complete|cancel>\n"
            " 4) pay <rideId> <method: upi|card|cash>\n"
            " 5) rate <rideId> <user|driver> <1-5>\n"
            " 6) summary\n"
            " 0) exit\n\n";

    string cmd;
    unordered_map<int, shared_ptr<Ride>> rideIndex;

    auto findRide = [&](int rid){
        auto it = rideIndex.find(rid);
        if(it==rideIndex.end()) throw NotFound("Ride not found");
        return it->second;
    };

    while(true){
        cout << ">> ";
        if(!(cin>>cmd)) break;
        try{
            if(cmd=="1" || cmd=="list-drivers"){
                cout << "-- Drivers --\n";
                auto all = drivers.all();
                sort(all.begin(), all.end(),[](auto&a,auto&b){return a->getId()<b->getId();});
                for(auto &d: all) printDriver(d);
            } else if(cmd=="2" || cmd=="request-ride"){
                int userId; double fx,fy,tx,ty; string type; cin>>userId>>fx>>fy>>tx>>ty>>type;
                auto ride = dispatch.requestRide(userId, Coord{fx,fy}, Coord{tx,ty}, type);
                rides.push_back(ride); rideIndex[ride->getId()] = ride;
                cout << "Ride created with id: "<< ride->getId() << ", type: "<< ride->fareName() << ", distance: "<< fixed<<setprecision(2)<< ride->getContext().km <<" km\n";
            } else if(cmd=="3" || cmd=="advance"){
                int rid; string action; cin>>rid>>action; auto r = findRide(rid);
                if(action=="accept") r->accept();
                else if(action=="start") r->start();
                else if(action=="complete") r->complete();
                else if(action=="cancel") r->cancel();
                else throw AppError("Unknown action");
                cout << "Status: "<< to_string(r->getStatus()) <<"\n";
            } else if(cmd=="4" || cmd=="pay"){
                int rid; string method; cin>>rid>>method; auto r = findRide(rid);
                if(r->getStatus()!=RideStatus::Completed) throw InvalidState("Pay only after completion");
                auto payer = PaymentFactory::create(method);
                payer->pay(r->getFare());
            } else if(cmd=="5" || cmd=="rate"){
                int rid; string who; int stars; cin>>rid>>who>>stars; auto r = findRide(rid);
                stars = max(1, min(5, stars));
                if(who=="user") r->getUser()->addRating(stars);
                else if(who=="driver") r->getDriver()->addRating(stars);
                else throw AppError("rate who must be user|driver");
                cout << "Thanks! Rated "<<who<<" "<<stars<<"/5\n";
            } else if(cmd=="6" || cmd=="summary"){
                cout << "-- Rides --\n";
                vector<shared_ptr<Ride>> list; for(auto &p: rideIndex) list.push_back(p.second);
                sort(list.begin(), list.end(), [](auto&a,auto&b){return a->getId()<b->getId();});
                for(auto &r: list){
                    cout << "#"<< r->getId() << " | User="<< r->getUser()->getName() << " | Driver="<< r->getDriver()->getName()
                         << " | Status="<< to_string(r->getStatus()) << " | FareType="<< r->fareName();
                    if(r->getStatus()==RideStatus::Completed) cout << " | Fare=Rs."<< fixed<<setprecision(2)<< r->getFare();
                    cout << "\n";
                }
            } else if(cmd=="0" || cmd=="exit"){
                cout << "Goodbye!\n"; break;
            } else {
                cout << "Unknown command.\n";
            }
        } catch(const exception&ex){
            cerr << "Error: "<< ex.what() << "\n";
        }
    }
    return 0;
}
