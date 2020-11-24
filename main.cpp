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

#define EPS 0.0001

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
        return glm::distance(pos, other.pos) < radius + other.radius;
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
        pos += vel * dt;
        old_vel = vel;
    }

    sf::CircleShape shape() const {
        auto s = sf::CircleShape(radius, 100);
        s.setOrigin(radius, radius);
        s.setPosition(pos.x, pos.y);
        return s;
    }

    static void resolve_position(PhysicsObject& a, PhysicsObject& b) {
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;
        float combined_depth = a.depth_into(b);
        a.pos += direction_b_to_a * combined_depth / 2.0f;
        b.pos += direction_a_to_b * combined_depth / 2.0f;
    }

    static void resolve_collision(PhysicsObject& a, PhysicsObject& b) {
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;

        vec2 a_reflection_normal = direction_b_to_a;
        vec2 b_reflection_normal = direction_a_to_b;

        vec2 a_reflected_vel = glm::reflect(a.old_vel, a_reflection_normal);
        vec2 b_reflected_vel = glm::reflect(b.old_vel, b_reflection_normal);

        float a_angle_vel_dir = glm::angle(glm::normalize(a.old_vel), direction_a_to_b);
        float b_angle_vel_dir = glm::angle(glm::normalize(b.old_vel), direction_b_to_a);
        bool a_transfers_velocity = a_angle_vel_dir < glm::radians(90.0f);
        bool b_transfers_velocity = b_angle_vel_dir < glm::radians(90.0f);
        float a_transfer_percent = 1.0f - a_angle_vel_dir / glm::radians(90.0f);
        float b_transfer_percent = 1.0f - b_angle_vel_dir / glm::radians(90.0f);

        vec2 new_a_vel = a.old_vel;
        vec2 new_b_vel = b.old_vel;
        float a_len = glm::length(a.old_vel);
        float b_len = glm::length(b.old_vel);
        if (a_transfers_velocity && !glm::isnan(a_len) && a_len > FLT_EPSILON) {
            new_b_vel += glm::normalize(direction_a_to_b) * a_transfer_percent * glm::length(new_a_vel);
            new_a_vel = glm::reflect(new_a_vel * (1.0f - a_transfer_percent), a_reflection_normal);
            //new_a_vel -= glm::normalize(b_reflected_vel) * a_transfer_percent;
        }
        if (b_transfers_velocity && !glm::isnan(b_len) && b_len > FLT_EPSILON) {
            new_a_vel += glm::normalize(direction_b_to_a) * b_transfer_percent * glm::length(new_b_vel);
            new_b_vel = glm::reflect(new_b_vel * (1.0f - b_transfer_percent), b_reflection_normal);
            //new_b_vel -= glm::normalize(a_reflected_vel) * b_transfer_percent;
        }
        a.vel = new_a_vel;
        b.vel = new_b_vel;
        resolve_position(a, b);

        /*float combined_mass = a.mass + b.mass;
        float combined_depth = a.depth_into(b);
        float a_mass_of_total = a.mass / combined_mass;
        float b_mass_of_total = b.mass / combined_mass;
        float a_diff = a_mass_of_total * combined_depth;
        float b_diff = b_mass_of_total * combined_depth;
        vec2 a_change = direction_b_to_a * b_diff;
        vec2 b_change = direction_a_to_b * a_diff;
        b.pos += b_change - 0.0001f;
        a.pos += a_change - 0.0001f;
        // velocity resolution
        vec2 a_vel = glm::normalize(a.vel);
        float angle = glm::angle(direction_a_to_b, a_vel);
        float angle_percent = fmodf(angle / (M_PI / 2.0f), 1.0f);
        //if (glm::abs(angle_percent) < 1.0f) {
        if (!glm::isnan(angle)) {
            std::cout << "before: angle_percent: " << int(angle_percent * 100.0f) << ", angle: " << angle / M_PI << "*pi, a.vel: " << a.vel << ", b.vel: " << b.vel << std::endl;


            vec2 a_mock_vel = direction_a_to_b * glm::length(a.vel) * angle_percent;
            vec2 b_mock_vel = direction_a_to_b * glm::length(a.vel) * (1.0f - angle_percent);

            //if (glm::abs(angle) < 0.001) {
                a.vel = a_mock_vel;
                b.vel += b_mock_vel;
            //} else {
            //}

            /
            vec2 d = direction_a_to_b * glm::length(a.vel);
            if (glm::abs(angle_percent) < 0.1) {
                b.vel += d;
                a.vel *= 0;
            } else {
                b.vel += d * angle_percent;
                a.vel -= d * angle_percent;
                a.vel *= 1.0f - angle_percent;
            }/

            std::cout << "after : angle_percent: " << int(angle_percent * 100.0f) << ", angle: " << angle / M_PI << "*pi, a.vel: " << a.vel << ", b.vel: " << b.vel << std::endl;
        }
        //a.vel = glm::reflect(a.vel, direction_b_to_a) * 0.01f + a.vel * 0.99f;
        //b.vel = glm::reflect(b.vel, direction_a_to_b) * 0.01f + b.vel * 0.99f;
    */
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
    objs.emplace_back(vec2(0, 300), vec2(100, 0), 10);
    for (size_t i = 0; i < 3; ++i) {
        objs.emplace_back(vec2(300 + i * 22, 310), vec2(0, 0), 10);
    }

    sf::Clock dt_clock;
    vec2 target = vec2(window.getSize().x, window.getSize().y) / 2.0f;
    sf::Font font;
    font.loadFromFile("font.ttf");
    bool pause = false;
    bool step = false;
    bool g_enabled = false;
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
           << "gravity: " << (g_enabled ? "ON" : "OFF") << "\n"
           << "sum of all forces: " << forces_sum << "\n";
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
            for (auto& obj : objs) {
                vec2 gravity = glm::normalize(target - obj.pos) * 1.f;
                if (g_enabled) {
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
                obj.apply_forces(dt);
            }
            if (step && !sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                step = false;
            }
        }
        window.clear();
        for (const auto& obj : objs) {
            window.draw(obj.shape());
            draw_force(obj.pos, obj.vel);
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
