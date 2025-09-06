#include <iostream>
#include <string>

// 产品基类
class Product {
public:
    virtual void Show() = 0;
    virtual ~Product() {}
};

// 具体产品A
class ProductA : public Product {
public:
    void Show() override {
        std::cout << "Product A" << std::endl;
    }
};

// 具体产品B
class ProductB : public Product {
public:
    void Show() override {
        std::cout << "Product B" << std::endl;
    }
};

// 工厂类
class SimpleFactory {
public:
    static Product* CreateProduct(const std::string& type) {
        if (type == "A") {
            return new ProductA();
        } else if (type == "B") {
            return new ProductB();
        }
        return nullptr;
    }
};

// 使用
int main() {
    Product* product = SimpleFactory::CreateProduct("A");
    if (product) {
        product->Show();
        delete product;
    }
    return 0;
}