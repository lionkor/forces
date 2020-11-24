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

class PhysicsObject {
public:
    vec2 pos;
    vec2 vel;
    float radius;
    float mass;
    vec2 acc { 0 };

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
    for (size_t i = 0; i < 1; ++i) {
        objs.emplace_back(vec2(200 + i * 20, 310), vec2(0), 10);
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
        std::stringstream ss;
        ss << "pause: " << (pause ? "YES" : "NO") << "\n"
           << "gravity: " << (g_enabled ? "ON" : "OFF") << "\n";
        status.setString(ss.str());
        status.setFillColor(sf::Color::White);
        status.setScale(0.8, 0.8);
        return status;
    };
    while (window.isOpen()) {
        float dt = dt_clock.restart().asSeconds();
        if (!pause || step) {
            for (auto& obj : objs) {
                vec2 gravity = glm::normalize(target - obj.pos) * 10.f;
                if (g_enabled) {
                    obj.acc += gravity / obj.mass;
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
            sf::VertexArray arr(sf::PrimitiveType::Lines);
            arr.append(sf::Vertex(sf::Vector2f(obj.pos.x, obj.pos.y), sf::Color::Green));
            vec2 p2 = obj.pos + (obj.vel);
            arr.append(sf::Vertex(sf::Vector2f(p2.x, p2.y), sf::Color::Red));
            sf::Text text(std::to_string(int(glm::length(obj.vel))), font);
            vec2 mid = (obj.pos + p2) / 2.0f;
            text.setPosition(mid.x, mid.y);
            text.setFillColor(sf::Color::White);
            text.setScale(0.3, 0.3);
            window.draw(obj.shape());
            window.draw(arr);
            window.draw(text);
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
