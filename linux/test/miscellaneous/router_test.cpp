#include <iostream>
#include <vector>
#include <string>

class Node {
public:
    Node() : children(CHARACTER_RANGE, nullptr) {}

    std::vector<Node*> children;
    std::string value; // Only used for leaf nodes

private:
    static const int CHARACTER_RANGE = 128; // ASCII character range
};

class Router {
public:
    Router() {
        root = new Node();
    }

    // Add a route to the router
    void addRoute(const std::string& route, const std::string& destination) {
        Node* node = root;
        for (char ch : route) {
            if (node->children[ch] == nullptr) {
                node->children[ch] = new Node();
            }
            node = node->children[ch];
        }
        node->value = destination;
    }

    // Route an incoming string to the corresponding destination
    void route(const std::string& input) {
        Node* node = root;
        for (char ch : input) {
            if (node->children[ch] == nullptr) {
                std::cout << "No route found for input: " << input << std::endl;
                return;
            }
            node = node->children[ch];
            if (!node->value.empty()) {
                std::cout << "Routed to: " << node->value << std::endl;
                return;
            }
        }
        std::cout << "No route found for input: " << input << std::endl;
    }

private:
    Node* root;
};

int main() {
    Router router;

    // Add some routes
    router.addRoute("/home", "HomeHandler");
    router.addRoute("/about", "AboutHandler");
    router.addRoute("/contact", "ContactHandler");
    router.addRoute("/users", "UsersHandler");

    // Test the router with different inputs
    router.route("/home");     // Output: Routed to: HomeHandler
    router.route("/about");    // Output: Routed to: AboutHandler
    router.route("/contact");  // Output: Routed to: ContactHandler
    router.route("/users");    // Output: Routed to: UsersHandler
    router.route("/products"); // Output: No route found for input: /products

    return 0;
}