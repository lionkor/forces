#include <SFML/Graphics.hpp>
#include <ctime>
#include <fstream>
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
    sf::CircleShape m_shape;

public:
    vec2 pos;
    vec2 vel;
    float radius;
    float mass;

    PhysicsObject(vec2 pos, vec2 vel, float radius)
        : pos(pos)
        , vel(vel)
        , radius(radius)
        , mass(M_PI * radius * radius) {
        m_shape = sf::CircleShape(radius, 100);
        m_shape.setOrigin(radius, radius);
    }
    virtual ~PhysicsObject() { }

    bool collides_with(const PhysicsObject& other) {
        // if the actual distance is less than the two radii together, they are colliding
        return glm::distance(pos, other.pos) + 0.01 < radius + other.radius;
    }
    float depth_into(const PhysicsObject& other) const {
        // difference between expected distance and actual distance, is negative if no collision
        return (radius + other.radius) - glm::distance(pos, other.pos);
    }

    void apply_forces(float dt) {
        // show forces
        if (glm::length(vel) < EPS || glm::isnan(glm::length(vel)) || glm::isinf(glm::length(vel))) {
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
        m_shape.setPosition(pos.x, pos.y);
    }

    sf::CircleShape& shape() {
        return m_shape;
    }

    static void resolve_position(PhysicsObject& a, PhysicsObject& b) {
        vec2 direction_a_to_b = glm::normalize(b.pos - a.pos);
        vec2 direction_b_to_a = -direction_a_to_b;
        float combined_mass = a.mass + b.mass;
        float combined_depth = a.depth_into(b);
        if (combined_depth < a.radius + b.radius - 0.1) {
            float a_mass_of_total = a.mass / combined_mass;
            float b_mass_of_total = b.mass / combined_mass;
            float a_diff = a_mass_of_total * combined_depth;
            float b_diff = b_mass_of_total * combined_depth;
            vec2 a_change = direction_b_to_a * b_diff;
            vec2 b_change = direction_a_to_b * a_diff;
            b.pos += b_change;
            a.pos += a_change;
        }
    }

    static inline void resolve_collision(PhysicsObject& a, PhysicsObject& b) {
        resolve_position(a, b);
        // aliases to make formulas look like common elastic collision formulas
        // which makes cross checking them for mistakes easier
        vec2& p1 = a.pos;
        vec2& p2 = b.pos;
        vec2& v1 = a.vel;
        vec2& v2 = b.vel;
        float& m1 = a.mass;
        float& m2 = b.mass;
        vec2 vp1p2 = p2 - p1;
        // normal vector
        vec2 vn = glm::normalize(vp1p2);
        // normal tangent vector
        vec2 vt = vec2(-vn.y, vn.x);
        // v1 projected onto vn
        float v1n = glm::dot(v1, vn);
        // v1 projected onto vt
        float v1t = glm::dot(v1, vt);
        // v2 projected onto vn
        float v2n = glm::dot(v2, vn);
        // v2 projected onto vt
        float v2t = glm::dot(v2, vt);
        // v1t and v2t stay unchanged, so we just alias them
        float v1t_p = v1t;
        float v2t_p = v2t;
        // one-dimensional collision formula
        float v1n_p = (v1n * (m1 - m2) + 2.0f * m2 * v2n) / (m1 + m2);
        float v2n_p = (v2n * (m2 - m1) + 2.0f * m1 * v1n) / (m1 + m2);
        // slap the scalars back in vectors
        vec2 v1n_vec = v1n_p * vn;
        vec2 v2n_vec = v2n_p * vn;
        vec2 v1t_vec = v1t_p * vt;
        vec2 v2t_vec = v2t_p * vt;
        // set final velocities (these are actually v1' and v2')
        a.vel = v1n_vec + v1t_vec;
        b.vel = v2n_vec + v2t_vec;
    }
};

float flt_epsilon(float x) {
    int exp;
    if (frexp(x, &exp) == 0.0)
        return FLT_MIN;
    return ldexp(FLT_EPSILON, exp - 1);
}

int main() {
    srand(time(nullptr));

    sf::Event event;

    std::vector<PhysicsObject> objs;
    objs.emplace_back(vec2(window.getSize().x / 2.0f, window.getSize().y / 2.0f), vec2(0, 0), 200);
    //objs.emplace_back(vec2(600, 200), vec2(0, 0), 100);
    //objs.emplace_back(vec2(900, 240), vec2(0, 0), 100);
    //objs.emplace_back(vec2(1100, 250), vec2(0, 0), 100);
    for (size_t i = 0; i < 200; ++i) {
        //objs.emplace_back(vec2(rand() % window.getSize().x, rand() % window.getSize().y), vec2((rand() % 100) - 50, (rand() % 100) - 50), rand() % 80 + 20);
        objs.emplace_back(vec2(rand() % window.getSize().x, rand() % window.getSize().y), vec2((rand() % 250) - 125), 10);
    }

    //std::ofstream logfile("averages.csv", std::ios::out);

    sf::Clock dt_clock;
    vec2 target = vec2(window.getSize().x, window.getSize().y) / 2.0f;
    sf::Font font;
    font.loadFromFile("font.ttf");
    bool pause = false;
    bool step = false;
    bool g_enabled = false;
    bool show_forces = false;
    float last_dt;
    auto make_status_text = [&]() -> sf::Text {
        sf::Text status;
        status.setFont(font);
        status.setPosition(5, 5);
        // sum all forces
        float p_sum = 0;
        float KE_sum = 0;
        float KE_eps_sum = 0;
        for (const auto& obj : objs) {
            float len = glm::length(obj.vel);
            p_sum += len * obj.mass;
            KE_sum += 0.5f * obj.mass * (len * len);
        }
        std::stringstream ss;
        ss << "pause: " << (pause ? "YES" : "NO") << "\n"
           << "show forces: " << (show_forces ? "ON" : "OFF") << "\n"
           << "gravity: " << (g_enabled ? "ON" : "OFF") << "\n"
           << "system's KE = " << int(KE_sum) << " (+-" << flt_epsilon(KE_sum) << ")\n"
           << "frame time (ms): " << last_dt * 1000.0f << "\n";
        //logfile << forces_sum / objs.size() << ",";
        status.setString(ss.str());
        status.setFillColor(sf::Color::Red);
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
        last_dt = dt;
        if (!pause || step) {
            const size_t n = 1;
            for (size_t i = 0; i < n; ++i) {
                for (auto& obj : objs) {
                    if (g_enabled) {
                        vec2 gravity = glm::normalize(target - obj.pos) * 5.f;
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
        for (auto& obj : objs) {
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
