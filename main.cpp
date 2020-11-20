#include <SFML/Graphics.hpp>
#include <ctime>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <random>

using glm::vec2;

class PhysicsObject {
public:
    vec2 pos;
    vec2 vel;
    float radius;
    float mass;

    PhysicsObject(vec2 pos, vec2 vel, float radius)
        : pos(pos)
        , vel(vel)
        , radius(radius)
        , mass(radius) { }
    virtual ~PhysicsObject() { }

    bool collides_with(const PhysicsObject& other) const {
        // if the actual distance is less than the two radii together, they are colliding
        return glm::distance(pos, other.pos) < radius + other.radius;
    }
    float depth_into(const PhysicsObject& other) const {
        // difference between expected distance and actual distance, is negative if no collision
        return (radius + other.radius) - glm::distance(pos, other.pos);
    }

    void apply_forces(float dt) {
        pos += vel * dt;
    }

    sf::CircleShape shape() const {
        auto s = sf::CircleShape(radius, 100);
        s.setOrigin(radius, radius);
        s.setPosition(pos.x, pos.y);
        return s;
    }

    static void resolve_collision(PhysicsObject& a, PhysicsObject& b) {
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;
        float combined_mass = a.mass + b.mass;
        float combined_depth = a.depth_into(b);
        float a_mass_of_total = a.mass / combined_mass;
        float b_mass_of_total = b.mass / combined_mass;
        float a_diff = a_mass_of_total * combined_depth;
        float b_diff = b_mass_of_total * combined_depth;
        vec2 a_change = direction_b_to_a * b_diff;
        vec2 b_change = direction_a_to_b * a_diff;
        b.pos += b_change;
        a.pos += a_change;
        a.vel = glm::reflect(a.vel, direction_b_to_a) * 0.01f + a.vel * 0.99f;
        b.vel = glm::reflect(b.vel, direction_a_to_b) * 0.01f + b.vel * 0.99f;
    }
};

int main() {
    srand(time(nullptr));
    sf::RenderWindow window(sf::VideoMode(1280, 720), "forces");

    sf::Event event;

    std::vector<PhysicsObject> objs;
    //objs.emplace_back(vec2(100, 200), vec2(0), 200);
    // objs.emplace_back(vec2(700, 200), vec2(0), 100);
    objs.emplace_back(vec2(700, 200), vec2(0), 40);
    for (size_t i = 0; i < 300; ++i) {
        objs.emplace_back(vec2(200 + i * 21, 300), vec2(0), 10);
    }

    sf::Clock dt_clock;
    vec2 target = vec2(window.getSize().x, window.getSize().y) / 2.0f;
    while (window.isOpen()) {
        float dt = dt_clock.restart().asSeconds();
        for (auto& obj : objs) {
            vec2 gravity = glm::normalize(target - obj.pos) * 50.f;
            obj.vel += gravity / obj.mass;
            obj.apply_forces(dt);
        }
        for (auto& obj : objs) {
            for (auto& other_obj : objs) {
                if (&obj == &other_obj) {
                    continue;
                }
                if (obj.collides_with(other_obj)) {
                    PhysicsObject::resolve_collision(obj, other_obj);
                }
            }
        }
        window.clear();
        for (const auto& obj : objs) {
            window.draw(obj.shape());
        }
        window.display();

        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed: {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                break;
            }
            case sf::Event::MouseMoved:
                target = vec2(event.mouseMove.x, event.mouseMove.y);
                break;
            default:
                break;
            }
        }
    }
}
