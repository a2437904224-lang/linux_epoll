#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// 抽象观察者
class Observer {
public:
    virtual void Update(const std::string& message) = 0;
    virtual ~Observer() {}
};

// 抽象发布者
class Subject {
public:
    virtual void Attach(Observer* observer) = 0;  // 注册观察者
    virtual void Detach(Observer* observer) = 0;  // 移除观察者
    virtual void Notify() = 0;                   // 通知所有观察者
    virtual ~Subject() {}
};

// 具体发布者
class ConcreteSubject : public Subject {
private:
    std::vector<Observer*> observers;  // 观察者列表
    std::string state;                 // 发布者的状态

public:
    void Attach(Observer* observer) override {
        observers.push_back(observer);
    }

    void Detach(Observer* observer) override {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

    void Notify() override {
        for (Observer* observer : observers) {
            observer->Update(state);
        }
    }

    void SetState(const std::string& newState) {
        state = newState;
        Notify();  // 状态改变时通知所有观察者
    }
};

// 具体观察者
class ConcreteObserver : public Observer {
private:
    std::string name;  // 观察者名称
    std::string observerState;

public:
    ConcreteObserver(const std::string& name) : name(name) {}

    void Update(const std::string& message) override {
        observerState = message;
        std::cout << "Observer " << name << " received update: " << observerState << std::endl;
    }
};

// 使用示例
int main() {
    // 创建发布者
    ConcreteSubject subject;

    // 创建观察者
    ConcreteObserver observer1("Observer1");
    ConcreteObserver observer2("Observer2");

    // 注册观察者
    subject.Attach(&observer1);
    subject.Attach(&observer2);

    // 改变状态并通知观察者
    subject.SetState("State1");
    subject.SetState("State2");

    // 移除一个观察者
    subject.Detach(&observer1);

    // 再次改变状态
    subject.SetState("State3");

    return 0;
}