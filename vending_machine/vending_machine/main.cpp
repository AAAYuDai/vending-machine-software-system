#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>

using namespace std;

// =========================
// 1. 数据模型
// =========================

struct Product {
    int id;
    string name;
    double price;
    int stock;

    bool isAvailable() const {
        return stock > 0;
    }
};

enum class OrderStatus {
    CREATED,    // 已创建，尚未支付
    PAID,       // 已支付
    COMPLETED,  // 购买完成
    CANCELLED,  // 已取消
    REFUNDED    // 已退款
};

struct Order {
    int id;
    int productId;
    int quantity;
    double amount;
    OrderStatus status;
};

// =========================
// 2. 商品数据仓库
// =========================

class ProductRepository {
private:
    vector<Product> products;

public:
    ProductRepository() {
        // 固定测试数据
        products = {
            {1, "可口可乐", 3.0, 10},
            {2, "矿泉水",   2.0, 20},
            {3, "薯片",     5.0, 8},
            {4, "巧克力",   6.0, 5}
        };
    }

    vector<Product>& getAll() {
        return products;
    }

    Product* findById(int id) {
        for (auto& product : products) {
            if (product.id == id) {
                return &product;
            }
        }
        return nullptr;
    }
};

// =========================
// 3. 支付服务：纯软件模拟
// =========================

class PaymentService {
private:
    bool refundAvailable = true;

public:
    bool pay(double amount) {
        cout << fixed << setprecision(2);
        cout << "应支付金额：¥" << amount << "\n";

        cout << "请选择模拟支付结果：\n";
        cout << "1. 支付成功\n";
        cout << "2. 支付失败\n";

        int choice;
        cin >> choice;

        if (choice == 1) {
            cout << "支付成功。\n";
            return true;
        }

        cout << "支付失败。\n";
        return false;
    }

    bool refund(double amount) {
        cout << fixed << setprecision(2);
        cout << "正在模拟退款 ¥" << amount << "...\n";

        if (refundAvailable) {
            cout << "退款成功。\n";
            return true;
        }

        cout << "退款失败。\n";
        return false;
    }
};

// =========================
// 4. 自动售货机核心业务
// =========================

class VendingMachine {
private:
    ProductRepository repository;
    PaymentService paymentService;

    Order* currentOrder = nullptr;
    int nextOrderId = 1;

public:

    // =========================
    // UC01：查看商品
    // =========================
    void showProducts() {
        cout << "\n========== 商品列表 ==========\n";

        cout << left
            << setw(6) << "编号"
            << setw(12) << "商品"
            << setw(10) << "价格"
            << setw(10) << "库存状态"
            << "\n";

        for (const auto& product : repository.getAll()) {

            cout << left
                << setw(6) << product.id
                << setw(12) << product.name
                << "¥"
                << setw(8)
                << fixed
                << setprecision(2)
                << product.price;

            if (product.stock > 0) {
                cout << "有货(" << product.stock << ")\n";
            }
            else {
                cout << "缺货\n";
            }
        }

        cout << "===============================\n";
    }

    // =========================
    // UC01 + UC02：选择并购买
    // =========================
    void buyProduct() {

        showProducts();

        int productId;

        cout << "请输入商品编号（输入 0 返回）：";
        cin >> productId;

        if (productId == 0) {
            return;
        }

        Product* product = repository.findById(productId);

        if (product == nullptr) {
            cout << "商品不存在，请重新选择。\n";
            return;
        }

        // 商品无库存
        if (!product->isAvailable()) {
            cout << "该商品暂不可购买：库存不足。\n";
            return;
        }

        int quantity;

        cout << "请输入购买数量：";
        cin >> quantity;

        if (quantity <= 0) {
            cout << "购买数量必须大于 0。\n";
            return;
        }

        // 检查库存
        if (quantity > product->stock) {
            cout << "库存不足，当前库存为 "
                << product->stock
                << "。\n";
            return;
        }

        // 计算金额
        double amount =
            product->price * quantity;

        // 创建订单
        Order order{
            nextOrderId++,
            product->id,
            quantity,
            amount,
            OrderStatus::CREATED
        };

        currentOrder = &order;

        cout << "\n---------- 当前订单 ----------\n";
        cout << "订单号：" << order.id << "\n";
        cout << "商品：" << product->name << "\n";
        cout << "数量：" << order.quantity << "\n";
        cout << "金额：¥"
            << fixed
            << setprecision(2)
            << order.amount
            << "\n";
        cout << "------------------------------\n";

        // -------------------------
        // 购买前取消
        // -------------------------

        cout << "1. 继续支付\n";
        cout << "2. 取消购买\n";
        cout << "请选择：";

        int action;
        cin >> action;

        if (action == 2) {

            cancelCurrentOrder();

            currentOrder = nullptr;

            return;
        }

        // -------------------------
        // UC02：模拟支付
        // -------------------------

        bool success =
            paymentService.pay(order.amount);

        if (!success) {

            // 支付失败：
            // 库存保持不变
            cout << "支付失败，库存不会发生变化。\n";

            currentOrder = nullptr;

            return;
        }

        // 支付成功
        order.status = OrderStatus::PAID;

        cout << "\n支付已经完成，当前订单状态：已支付。\n";

        // -------------------------
        // UC03：支付后取消
        // -------------------------

        cout << "1. 完成购买\n";
        cout << "2. 取消购买并退款\n";
        cout << "请选择：";

        int afterPayChoice;
        cin >> afterPayChoice;

        if (afterPayChoice == 2) {

            bool refunded =
                paymentService.refund(order.amount);

            if (!refunded) {

                cout << "退款异常，订单状态保留为已支付。\n";

                currentOrder = nullptr;

                return;
            }

            order.status = OrderStatus::REFUNDED;
            order.status = OrderStatus::CANCELLED;

            // 此时库存还没有扣减
            cout << "订单已取消并退款，库存保持不变。\n";

            currentOrder = nullptr;

            return;
        }

        // -------------------------
        // 最终完成购买
        // -------------------------

        // 支付成功 + 最终确认后才扣库存
        product->stock -= quantity;

        order.status = OrderStatus::COMPLETED;

        cout << "扣减库存成功，当前库存："
            << product->stock
            << "\n";

        cout << "购买成功！订单号："
            << order.id
            << "\n";

        currentOrder = nullptr;
    }

    // =========================
    // UC03：取消购买并退款
    // =========================
    void cancelCurrentOrder() {

        if (currentOrder == nullptr) {
            cout << "当前没有可取消的订单。\n";
            return;
        }

        cout << "确认取消订单？\n";
        cout << "1. 是\n";
        cout << "2. 否\n";

        int choice;
        cin >> choice;

        if (choice != 1) {

            cout << "已放弃取消，返回当前购买流程。\n";

            return;
        }

        // 如果已经支付，需要退款
        if (currentOrder->status == OrderStatus::PAID) {

            bool refunded =
                paymentService.refund(
                    currentOrder->amount
                );

            if (!refunded) {

                cout << "退款异常，订单状态保留。\n";

                return;
            }

            currentOrder->status =
                OrderStatus::REFUNDED;
        }

        currentOrder->status =
            OrderStatus::CANCELLED;

        cout << "订单已取消。\n";
    }

    // =========================
    // UC04：管理员补货
    // =========================
    void adminRestock() {

        showProducts();

        int productId;

        cout << "请输入需要补货的商品编号（输入 0 返回）：";
        cin >> productId;

        if (productId == 0) {
            return;
        }

        Product* product =
            repository.findById(productId);

        if (product == nullptr) {

            cout << "商品不存在，操作结束。\n";

            return;
        }

        int quantity;

        cout << "请输入补货数量：";
        cin >> quantity;

        if (quantity <= 0) {

            cout << "补货数量非法，库存不会修改。\n";

            return;
        }

        product->stock += quantity;

        cout << "补货成功。\n";
        cout << "商品：" << product->name << "\n";
        cout << "当前库存：" << product->stock << "\n";
    }

    // 管理员查看库存
    void adminView() {

        showProducts();
    }
};

// =========================
// 5. 主程序
// =========================

void clearInput() {

    cin.clear();

    cin.ignore(
        numeric_limits<streamsize>::max(),
        '\n'
    );
}

int main() {

    VendingMachine machine;

    while (true) {

        cout << "\n=================================\n";
        cout << "       自动售货机软件系统\n";
        cout << "=================================\n";

        cout << "1. 用户模式\n";
        cout << "2. 管理员模式\n";
        cout << "0. 退出系统\n";

        cout << "请选择：";

        int roleChoice;

        if (!(cin >> roleChoice)) {

            cout << "输入格式错误，请重新输入。\n";

            clearInput();

            continue;
        }

        // =========================
        // 退出
        // =========================

        if (roleChoice == 0) {

            cout << "系统已退出。\n";

            break;
        }

        // =========================
        // 用户模式
        // =========================

        if (roleChoice == 1) {

            while (true) {

                cout << "\n---------- 用户模式 ----------\n";

                cout << "1. 查看商品\n";
                cout << "2. 购买商品\n";
                cout << "0. 返回上一级\n";

                cout << "请选择：";

                int choice;

                cin >> choice;

                if (choice == 0) {
                    break;
                }

                if (choice == 1) {

                    machine.showProducts();

                }
                else if (choice == 2) {

                    machine.buyProduct();

                }
                else {

                    cout << "无效选择，请重新输入。\n";
                }
            }
        }

        // =========================
        // 管理员模式
        // =========================

        else if (roleChoice == 2) {

            /*
             * 需求文档没有规定具体的管理员认证方式。
             * 这里使用固定 PIN，仅用于课程演示，
             * 实现“普通用户不能进入管理员区域”的基本要求。
             */

            const string ADMIN_PIN = "1234";

            string pin;

            cout << "请输入管理员 PIN：";

            cin >> pin;

            if (pin != ADMIN_PIN) {

                cout << "管理员认证失败，不能进入管理区域。\n";

                continue;
            }

            while (true) {

                cout << "\n---------- 管理员模式 ----------\n";

                cout << "1. 查看商品及库存\n";
                cout << "2. 商品补货\n";
                cout << "0. 返回上一级\n";

                cout << "请选择：";

                int choice;

                cin >> choice;

                if (choice == 0) {
                    break;
                }

                if (choice == 1) {

                    machine.adminView();

                }
                else if (choice == 2) {

                    machine.adminRestock();

                }
                else {

                    cout << "无效选择，请重新输入。\n";
                }
            }
        }

        else {

            cout << "无效选择，请重新输入。\n";
        }
    }

    return 0;
}