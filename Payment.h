#ifndef PAYMENT_H
#define PAYMENT_H

#include <iostream>
#include <memory>

class PaymentProcessor {
public:
    virtual void pay(double amt) = 0;
    virtual ~PaymentProcessor() = default;
};

class CardPayment : public PaymentProcessor {
public:
    void pay(double amt) override {
        std::cout << "Paid " << amt << " via Card\n";
    }
};

class WalletPayment : public PaymentProcessor {
public:
    void pay(double amt) override {
        std::cout << "Paid " << amt << " via Wallet\n";
    }
};

class PaymentFactory {
public:
    static std::unique_ptr<PaymentProcessor> getProcessor(const std::string &type) {
        if(type=="card") return std::make_unique<CardPayment>();
        return std::make_unique<WalletPayment>();
    }
};

#endif
