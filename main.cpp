#include <SFML/Graphics.hpp>
#include <ctime>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <iostream>
#include <random>
#include <sstream>

using glm::vec2;

sf::RenderWindow window(sf::VideoMode(1280, 720), "forces");

std::ostream& operator<<(std::ostream& os, const vec2& v) {
    return os << "(" << v.x << ", " << v.y << ")";
}

#define EPS 0

class PhysicsObject {
public:
    vec2 pos;
    vec2 vel;
    float radius;
    float mass;
    vec2 old_vel;

    PhysicsObject(vec2 pos, vec2 vel, float radius)
        : pos(pos)
        , vel(vel)
        , radius(radius)
        , mass(radius) { }
    virtual ~PhysicsObject() { }

    bool collides_with(const PhysicsObject& other) const {
        // if the actual distance is less than the two radii together, they are colliding
        return glm::distance(pos, other.pos) + 0.01 < radius + other.radius;
    }
    float depth_into(const PhysicsObject& other) const {
        // difference between expected distance and actual distance, is negative if no collision
        return (radius + other.radius) - glm::distance(pos, other.pos);
    }

    void apply_forces(float dt) {
        // show forces
        if (glm::length(vel) < EPS || glm::isnan(glm::length(vel))) {
            vel = vec2(0);
        }
        vec2 next_pos = pos + vel * dt;
        if (next_pos.x < 0) {
            vel = glm::reflect(vel, vec2(1, 0));
        }
        if (next_pos.y < 0) {
            vel = glm::reflect(vel, vec2(0, 1));
        }
        if (next_pos.x > window.mapPixelToCoords(sf::Vector2i(window.getSize())).x) {
            vel = glm::reflect(vel, vec2(-1, 0));
        }
        if (next_pos.y > window.mapPixelToCoords(sf::Vector2i(window.getSize())).y) {
            vel = glm::reflect(vel, vec2(0, -1));
        }
        pos += vel * dt;
        old_vel = vel;
    }

    sf::CircleShape shape() const {
        auto s = sf::CircleShape(radius, 100);
        s.setPosition(pos.x, pos.y);
        s.setOrigin(radius, radius);
        return s;
    }

    static void resolve_position(PhysicsObject& a, PhysicsObject& b) {
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;
        float combined_depth = a.depth_into(b);
        a.pos += direction_b_to_a * (combined_depth / 2.f + FLT_EPSILON);
        b.pos += direction_a_to_b * (combined_depth / 2.f + FLT_EPSILON);
    }

    static void resolve_collision(PhysicsObject& a, PhysicsObject& b) {
        resolve_position(a, b);
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;

        vec2 a_reflection_normal = direction_b_to_a;
        vec2 b_reflection_normal = direction_a_to_b;

        float a_angle = glm::angle(a.old_vel, a_reflection_normal);
        bool a_transfers = a_angle > M_PI / 2.0f;

        float b_angle = glm::angle(b.old_vel, b_reflection_normal);
        bool b_transfers = b_angle > M_PI / 2.0f;

        if (a_transfers || b_transfers) {
            a.vel = vec2(0);
            b.vel = vec2(0);
        }
        if (a_transfers) {
            std::cout << "a" << std::endl;
            float a_transfer_percent = 1.0f - (a_angle / glm::radians(180.f) - 0.5f);
            b.vel += a.old_vel * a_transfer_percent;;
            a.vel += glm::reflect(a.old_vel * (1.0f - a_transfer_percent), a_reflection_normal);
        }
        if (b_transfers) {
            std::cout << "a" << std::endl;
            float b_transfer_percent = 1.0f - (b_angle / glm::radians(180.f) - 0.5f);
            vec2 change = b.old_vel * b_transfer_percent;
            a.vel += change;
            b.vel += glm::reflect(b.old_vel * (1.0f - b_transfer_percent), b_reflection_normal);
        }
    }
};

int main() {
    srand(time(nullptr));

    sf::Event event;

    std::vector<PhysicsObject> objs;
    //objs.emplace_back(vec2(100, 200), vec2(300, 0), 100);
    //objs.emplace_back(vec2(600, 200), vec2(0, 0), 100);
    //objs.emplace_back(vec2(900, 240), vec2(0, 0), 100);
    //objs.emplace_back(vec2(1100, 250), vec2(0, 0), 100);
    for (size_t i = 0; i < 5; ++i) {
        objs.emplace_back(vec2(0, 100 + 100 * i), vec2(300, 0), 10);
        objs.emplace_back(vec2(800, 100 + 100 * i), vec2(0, 0), 10);
    }

    sf::Clock dt_clock;
    vec2 target = vec2(window.getSize().x, window.getSize().y) / 2.0f;
    sf::Font font;
    font.loadFromFile("font.ttf");
    bool pause = false;
    bool step = false;
    bool g_enabled = false;
    bool show_forces = false;
    auto make_status_text = [&]() -> sf::Text {
        sf::Text status;
        status.setFont(font);
        status.setPosition(5, 5);
        // sum all forces
        float forces_sum = 0;
        for (const auto& obj : objs) {
            float len = glm::length(obj.vel);
            if (!glm::isnan(len)) {
                forces_sum += len;
            }
        }
        std::stringstream ss;
        ss << "pause: " << (pause ? "YES" : "NO") << "\n"
           << "show forces: " << (show_forces ? "ON" : "OFF") << "\n"
           << "gravity: " << (g_enabled ? "ON" : "OFF") << "\n"
           << "avg of all forces: " << forces_sum / objs.size() << "\n";
        status.setString(ss.str());
        status.setFillColor(sf::Color::White);
        status.setScale(0.8, 0.8);
        return status;
    };
    auto draw_force = [&](const vec2& pos, const vec2& force) {
        sf::VertexArray arr(sf::PrimitiveType::Lines);
        arr.append(sf::Vertex(sf::Vector2f(pos.x, pos.y), sf::Color::Green));
        vec2 p2 = pos + force;
        arr.append(sf::Vertex(sf::Vector2f(p2.x, p2.y), sf::Color::Red));
        sf::Text text(std::to_string(int(glm::length(force))), font);
        vec2 mid = (pos + p2) / 2.0f;
        text.setPosition(mid.x, mid.y);
        text.setFillColor(sf::Color::Cyan);
        text.setScale(0.3, 0.3);
        window.draw(arr);
        window.draw(text);
    };
    while (window.isOpen()) {
        float dt = dt_clock.restart().asSeconds();
        if (!pause || step) {
            const size_t n = 1;
            for (size_t i = 0; i < n; ++i) {
                for (auto& obj : objs) {
                    if (g_enabled) {
                        vec2 gravity = glm::normalize(target - obj.pos) * 1.f;
                        obj.vel += gravity / obj.mass;
                    }
                    for (auto& other_obj : objs) {
                        if (&obj == &other_obj) {
                            continue;
                        }
                        if (obj.collides_with(other_obj)) {
                            PhysicsObject::resolve_collision(obj, other_obj);
                        }
                    }
                }
                for (auto& obj : objs) {
                    obj.apply_forces(dt / float(n));
                }
            }
            if (step && !sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                step = false;
            }
        }
        window.clear();
        for (const auto& obj : objs) {
            window.draw(obj.shape());
            if (show_forces) {
                draw_force(obj.pos, obj.vel);
            }
        }
        window.draw(make_status_text());
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
                if (event.key.code == sf::Keyboard::Space) {
                    pause = !pause;
                }
                if (pause && (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::Up)) {
                    step = true;
                }
                if (event.key.code == sf::Keyboard::G) {
                    g_enabled = !g_enabled;
                }
                if (event.key.code == sf::Keyboard::A) {
                    auto real_pos = window.mapPixelToCoords(sf::Mouse::getPosition());
                    objs.emplace_back(vec2(real_pos.x, real_pos.y), vec2(0), 10);
                }
                if (event.key.code == sf::Keyboard::F) {
                    show_forces = !show_forces;
                }
                break;
            }
            case sf::Event::MouseMoved: {
                auto real_vec = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                target = vec2(real_vec.x, real_vec.y);
                break;
            }
            default:
                break;
            }
        }
    }
}
