#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <windows.h>
#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <string>
#include <fstream>

// Game States
enum class GameState { Menu, Playing, GameOver, Store };

// Abstract Base Class
class Entity {
public:
    virtual ~Entity() = default;
    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) const = 0;
    virtual sf::FloatRect getGlobalBounds() const = 0;
};

// Player Class inheriting from Entity
class Player : public Entity {
private:
    float velocityY;
    float gravity;
    float jumpForce;
    bool isGrounded;
    float groundLevel;

    sf::RectangleShape hitbox;
    
    // Stick figure components
    sf::CircleShape head;
    sf::RectangleShape body;
    sf::RectangleShape leftArm;
    sf::RectangleShape rightArm;
    sf::RectangleShape leftLeg;
    sf::RectangleShape rightLeg;
    float runAnimTimer;
    int jumpCount;

public:
    Player(float startX, float startY, sf::Color stickColor) 
        : velocityY(0.0f), gravity(1500.0f), jumpForce(-650.0f), isGrounded(false), groundLevel(startY), runAnimTimer(0.0f), jumpCount(0) {
        
        hitbox.setSize(sf::Vector2f(50.0f, 50.0f));
        hitbox.setFillColor(sf::Color::Transparent); 
        hitbox.setPosition(startX, startY);

        head.setRadius(8.0f);
        head.setFillColor(stickColor);
        head.setOrigin(8.0f, 8.0f);

        body.setSize(sf::Vector2f(4.0f, 20.0f));
        body.setFillColor(stickColor);
        body.setOrigin(2.0f, 0.0f);

        leftArm.setSize(sf::Vector2f(4.0f, 16.0f));
        leftArm.setFillColor(stickColor);
        leftArm.setOrigin(2.0f, 2.0f); 
        
        rightArm.setSize(sf::Vector2f(4.0f, 16.0f));
        rightArm.setFillColor(stickColor);
        rightArm.setOrigin(2.0f, 2.0f);

        leftLeg.setSize(sf::Vector2f(4.0f, 18.0f));
        leftLeg.setFillColor(stickColor);
        leftLeg.setOrigin(2.0f, 2.0f);

        rightLeg.setSize(sf::Vector2f(4.0f, 18.0f));
        rightLeg.setFillColor(stickColor);
        rightLeg.setOrigin(2.0f, 2.0f);
    }

    void handleInput(sf::Keyboard::Key key) {
        if (key == sf::Keyboard::Space || key == sf::Keyboard::W || key == sf::Keyboard::Up) {
            jump();
        }
    }

    void jump() {
        if (jumpCount < 2) {
            velocityY = jumpForce;
            isGrounded = false;
            jumpCount++;
        }
    }

    void update(float deltaTime) override {
        if (!isGrounded) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                velocityY += gravity * 3.5f * deltaTime; // Fast fall
            } else {
                velocityY += gravity * deltaTime;
            }
        }

        float moveX = 0.0f;
        float moveSpeed = 300.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            moveX = -moveSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            moveX = moveSpeed * deltaTime;
        }

        hitbox.move(moveX, velocityY * deltaTime);

        // Screen bounds checking
        if (hitbox.getPosition().x < 0.0f) hitbox.setPosition(0.0f, hitbox.getPosition().y);
        if (hitbox.getPosition().x + 50.0f > 800.0f) hitbox.setPosition(800.0f - 50.0f, hitbox.getPosition().y);

        if (hitbox.getPosition().y >= groundLevel) {
            hitbox.setPosition(hitbox.getPosition().x, groundLevel);
            velocityY = 0.0f;
            isGrounded = true;
            jumpCount = 0;
        } else {
            isGrounded = false;
        }

        // Stick Figure Animation
        sf::Vector2f pos = hitbox.getPosition();
        float cx = pos.x + 25.0f;
        float cy = pos.y;

        head.setPosition(cx, cy + 10.0f);
        body.setPosition(cx, cy + 18.0f);
        leftArm.setPosition(cx, cy + 20.0f);
        rightArm.setPosition(cx, cy + 20.0f);
        leftLeg.setPosition(cx, cy + 36.0f);
        rightLeg.setPosition(cx, cy + 36.0f);

        if (isGrounded) {
            if (moveX != 0.0f) {
                runAnimTimer += deltaTime * 20.0f; // faster anim when moving horizontally
            } else {
                runAnimTimer += deltaTime * 10.0f; 
            }
            float swing = std::sin(runAnimTimer) * 45.0f; 
            
            leftLeg.setRotation(swing);
            rightLeg.setRotation(-swing);
            leftArm.setRotation(-swing);
            rightArm.setRotation(swing);
        } else {
            leftArm.setRotation(-150.0f); 
            rightArm.setRotation(150.0f);
            leftLeg.setRotation(30.0f);  
            rightLeg.setRotation(-45.0f); 
        }
    }

    void draw(sf::RenderWindow& window) const override {
        window.draw(leftArm);
        window.draw(leftLeg);
        window.draw(body);
        window.draw(head);
        window.draw(rightLeg);
        window.draw(rightArm);
    }

    sf::FloatRect getGlobalBounds() const override {
        return hitbox.getGlobalBounds();
    }
};

// Abstract Obstacle Base Class (Inherits from Entity)
class Obstacle : public Entity {
protected:
    float speed;
    bool passedPlayer;
public:
    Obstacle(float speed) : speed(speed), passedPlayer(false) {}
    
    virtual bool isOffScreen() const = 0;
    
    bool checkPassed(float playerX) {
        if (!passedPlayer && getGlobalBounds().left + getGlobalBounds().width < playerX) {
            passedPlayer = true;
            return true;
        }
        return false;
    }
};

// Spike Class (Triangle)
class Spike : public Obstacle {
private:
    sf::ConvexShape triangle;
public:
    Spike(float startX, float groundLevel, float speed) : Obstacle(speed) {
        triangle.setPointCount(3);
        triangle.setPoint(0, sf::Vector2f(20.0f, 0.0f));
        triangle.setPoint(1, sf::Vector2f(40.0f, 40.0f));
        triangle.setPoint(2, sf::Vector2f(0.0f, 40.0f));
        triangle.setFillColor(sf::Color::Red);
        triangle.setPosition(startX, groundLevel + 50.0f - 40.0f);
    }
    void update(float deltaTime) override { triangle.move(-speed * deltaTime, 0); }
    void draw(sf::RenderWindow& window) const override { window.draw(triangle); }
    sf::FloatRect getGlobalBounds() const override { return triangle.getGlobalBounds(); }
    bool isOffScreen() const override { return triangle.getPosition().x + triangle.getGlobalBounds().width < 0; }
};

// Boulder Class (Circle)
class Boulder : public Obstacle {
private:
    sf::CircleShape circle;
public:
    Boulder(float startX, float groundLevel, float speed) : Obstacle(speed) {
        circle.setRadius(20.0f);
        circle.setOrigin(20.0f, 20.0f);
        circle.setFillColor(sf::Color(139, 69, 19)); // Brown
        circle.setPosition(startX + 20.0f, groundLevel + 50.0f - 20.0f);
    }
    void update(float deltaTime) override {
        circle.move(-speed * deltaTime, 0);
        circle.rotate(-speed * deltaTime * 3.0f); 
    }
    void draw(sf::RenderWindow& window) const override { window.draw(circle); }
    sf::FloatRect getGlobalBounds() const override { return circle.getGlobalBounds(); }
    bool isOffScreen() const override { return circle.getPosition().x + circle.getGlobalBounds().width < 0; }
};

// Crusher Class (Rectangle) - NOW STATIC HEIGHT
class Crusher : public Obstacle {
private:
    sf::RectangleShape rect;
public:
    Crusher(float startX, float groundLevel, float speed) : Obstacle(speed) {
        rect.setSize(sf::Vector2f(40.0f, 150.0f));
        rect.setFillColor(sf::Color(105, 105, 105)); // Gray
        rect.setPosition(startX, groundLevel + 50.0f - 150.0f); // Rest on ground
    }
    void update(float deltaTime) override {
        rect.move(-speed * deltaTime, 0);
        // Removed sine wave bouncing logic
    }
    void draw(sf::RenderWindow& window) const override { window.draw(rect); }
    sf::FloatRect getGlobalBounds() const override { return rect.getGlobalBounds(); }
    bool isOffScreen() const override { return rect.getPosition().x + rect.getGlobalBounds().width < 0; }
};

// Coin Class
class Coin : public Entity {
private:
    sf::CircleShape circle;
    float speed;
    bool collected;
public:
    Coin(float startX, float yPos, float speed) : speed(speed), collected(false) {
        circle.setRadius(12.0f);
        circle.setOrigin(12.0f, 12.0f);
        circle.setFillColor(sf::Color::Yellow);
        circle.setOutlineColor(sf::Color(255, 215, 0)); // Golden outline
        circle.setOutlineThickness(2.0f);
        circle.setPosition(startX, yPos);
    }
    void update(float deltaTime) override {
        circle.move(-speed * deltaTime, 0);
    }
    void draw(sf::RenderWindow& window) const override { window.draw(circle); }
    sf::FloatRect getGlobalBounds() const override { return circle.getGlobalBounds(); }
    bool isOffScreen() const { return circle.getPosition().x + circle.getGlobalBounds().width < 0; }
    bool isCollected() const { return collected; }
    void collect() { collected = true; }
};

// Background Elements
class Star {
public:
    sf::CircleShape dot;
    float speed;
    
    Star(float x, float y, float speed) : speed(speed) {
        dot.setRadius((rand() % 15 + 10) / 10.0f); // 1.0f to 2.5f
        dot.setPosition(x, y);
        dot.setFillColor(sf::Color(255, 255, 255, rand() % 155 + 100)); // 100 to 255 alpha
    }
    void update(float deltaTime) { dot.move(-speed * deltaTime, 0); }
    void draw(sf::RenderWindow& window) const { window.draw(dot); }
    bool isOffScreen() const { return dot.getPosition().x < -10.0f; }
};

class Mountain {
public:
    sf::ConvexShape peak;
    float speed;
    
    Mountain(float startX, float groundY, float speed) : speed(speed) {
        float width = (rand() % 200) + 150.0f;
        float height = (rand() % 150) + 100.0f;
        
        peak.setPointCount(3);
        peak.setPoint(0, sf::Vector2f(0.0f, height));
        peak.setPoint(1, sf::Vector2f(width / 2.0f, 0.0f));
        peak.setPoint(2, sf::Vector2f(width, height));
        
        peak.setFillColor(sf::Color(40, 40, 55)); // Dark mountainous color
        peak.setPosition(startX, groundY + 50.0f - height);
    }
    
    void update(float deltaTime) { peak.move(-speed * deltaTime, 0); }
    void draw(sf::RenderWindow& window) const { window.draw(peak); }
    bool isOffScreen() const { return peak.getPosition().x + peak.getGlobalBounds().width < 0; }
};

// Game Manager Class
class Game {
private:
    sf::RenderWindow window;
    
    std::vector<std::unique_ptr<Entity>> gameEntities;
    std::vector<Star> stars;
    std::vector<Mountain> mountains;
    std::unique_ptr<Player> player;
    
    sf::Clock clock;
    float spawnTimer;
    float spawnInterval;
    
    std::mt19937 rng;
    
    const float groundY = 450.0f;
    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;

    GameState currentState;
    int score;
    int highScore;
    int coins;

    sf::Color playerColor;
    bool ownsBlue;
    bool ownsPurple;

    sf::Font font;
    sf::Text titleText;
    sf::Text instructionText;
    sf::Text scoreText;
    sf::Text storeText;

    void initUI() {
        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
            if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf")) {
                std::cerr << "Warning: Could not load any system fonts.\n";
            }
        }

        titleText.setFont(font);
        titleText.setCharacterSize(60);
        titleText.setFillColor(sf::Color::White);
        titleText.setStyle(sf::Text::Bold);
        
        instructionText.setFont(font);
        instructionText.setCharacterSize(30);
        instructionText.setFillColor(sf::Color::Yellow);

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(20.0f, 20.0f);

        storeText.setFont(font);
        storeText.setCharacterSize(28);
        storeText.setFillColor(sf::Color::White);
    }

    void resetGame() {
        gameEntities.clear();
        player = std::make_unique<Player>(100.0f, groundY, playerColor);
        
        spawnTimer = 0.0f;
        score = 0;
    }

    void spawnEntity() {
        float baseSpeed = 250.0f + (score * 5.0f); 
        float maxSpeed = 450.0f + (score * 5.0f);
        
        std::uniform_real_distribution<float> speedDist(baseSpeed, maxSpeed);
        std::uniform_int_distribution<int> typeDist(0, 10);
        
        float speed = speedDist(rng);
        int type = typeDist(rng);
        
        if (type < 3) {
            gameEntities.push_back(std::make_unique<Spike>(screenWidth, groundY, speed));
        } else if (type < 6) {
            gameEntities.push_back(std::make_unique<Boulder>(screenWidth, groundY, speed));
        } else if (type < 8) {
            gameEntities.push_back(std::make_unique<Crusher>(screenWidth, groundY, speed));
        } else {
            // Spawn Coin high up or low
            std::uniform_real_distribution<float> yDist(groundY - 120.0f, groundY - 20.0f);
            gameEntities.push_back(std::make_unique<Coin>(screenWidth, yDist(rng), speed));
        }
    }

public:
    Game() 
        : window(sf::VideoMode(800, 600), "Stickman Survivor"), 
          spawnTimer(0.0f), 
          spawnInterval(1.5f),
          currentState(GameState::Menu),
          score(0),
          highScore(0),
          coins(0),
          playerColor(sf::Color::Green),
          ownsBlue(false),
          ownsPurple(false) {
        
        window.setFramerateLimit(60);
        std::random_device rd;
        rng.seed(rd());
        
        initUI();
        loadGame();
        resetGame();

        // Initial background fill
        for (int i = 0; i < 100; i++) {
            float x = (rand() % 800);
            float y = (rand() % (int)groundY);
            stars.push_back(Star(x, y, (rand() % 20) + 10.0f));
        }
        for (int i = 0; i < 5; i++) {
            float x = (rand() % 1000) - 200.0f;
            mountains.push_back(Mountain(x, groundY, (rand() % 30) + 40.0f));
        }
    }

    void loadGame() {
        std::ifstream file("save.txt");
        if (file.is_open()) {
            file >> highScore >> coins;
            int b, p;
            file >> b >> p;
            ownsBlue = (b == 1);
            ownsPurple = (p == 1);
            int r, g, bl;
            if (file >> r >> g >> bl) {
                playerColor = sf::Color(r, g, bl);
            }
            file.close();
        }
    }

    void saveGame() {
        std::ofstream file("save.txt");
        if (file.is_open()) {
            file << highScore << "\n" << coins << "\n" 
                 << (ownsBlue ? 1 : 0) << "\n" << (ownsPurple ? 1 : 0) << "\n" 
                 << (int)playerColor.r << " " << (int)playerColor.g << " " << (int)playerColor.b << "\n";
            file.close();
        }
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            update();
            render();
        }
    }

private:
    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (currentState == GameState::Menu) {
                    if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Enter) {
                        resetGame();
                        currentState = GameState::Playing;
                    } else if (event.key.code == sf::Keyboard::S) {
                        currentState = GameState::Store;
                    }
                } 
                else if (currentState == GameState::GameOver) {
                    if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Enter) {
                        resetGame();
                        currentState = GameState::Playing;
                    } else if (event.key.code == sf::Keyboard::M || event.key.code == sf::Keyboard::Escape) {
                        currentState = GameState::Menu;
                    }
                }
                else if (currentState == GameState::Store) {
                    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::M) {
                        currentState = GameState::Menu;
                    } else if (event.key.code == sf::Keyboard::Num1) {
                        playerColor = sf::Color::Green;
                        saveGame();
                    } else if (event.key.code == sf::Keyboard::Num2) {
                        if (ownsBlue) {
                            playerColor = sf::Color::Blue;
                            saveGame();
                        } else if (coins >= 10) {
                            coins -= 10;
                            ownsBlue = true;
                            playerColor = sf::Color::Blue;
                            saveGame();
                        }
                    } else if (event.key.code == sf::Keyboard::Num3) {
                        if (ownsPurple) {
                            playerColor = sf::Color::Magenta;
                            saveGame();
                        } else if (coins >= 20) {
                            coins -= 20;
                            ownsPurple = true;
                            playerColor = sf::Color::Magenta;
                            saveGame();
                        }
                    }
                }
                else if (currentState == GameState::Playing) {
                    player->handleInput(event.key.code);
                }
            }
        }
    }

    void update() {
        float deltaTime = clock.restart().asSeconds();
        
        // Background parallax updates
        if (rand() % 100 < 5) stars.push_back(Star(810.0f, (rand() % (int)groundY), (rand() % 20) + 10.0f));
        if (rand() % 1000 < 5) mountains.push_back(Mountain(810.0f, groundY, (rand() % 30) + 40.0f));

        for (auto it = stars.begin(); it != stars.end();) {
            it->update(deltaTime);
            if (it->isOffScreen()) it = stars.erase(it);
            else ++it;
        }
        for (auto it = mountains.begin(); it != mountains.end();) {
            it->update(deltaTime);
            if (it->isOffScreen()) it = mountains.erase(it);
            else ++it;
        }

        if (currentState != GameState::Playing) return;

        spawnTimer += deltaTime;
        if (spawnTimer >= spawnInterval) {
            spawnEntity();
            spawnTimer = 0.0f;
            
            std::uniform_real_distribution<float> intervalDist(1.0f, 2.5f);
            spawnInterval = intervalDist(rng);
        }

        player->update(deltaTime);

        auto it = gameEntities.begin();
        while (it != gameEntities.end()) {
            (*it)->update(deltaTime);
            bool removeEntity = false;
            
            Obstacle* obs = dynamic_cast<Obstacle*>(it->get());
            if (obs) {
                if (obs->isOffScreen()) {
                    removeEntity = true;
                }
                else if (player->getGlobalBounds().intersects(obs->getGlobalBounds())) {
                    currentState = GameState::GameOver;
                    return;
                }
                else if (obs->checkPassed(player->getGlobalBounds().left)) {
                    score++;
                    if (score > highScore) { highScore = score; saveGame(); }
                }
            }

            Coin* coin = dynamic_cast<Coin*>(it->get());
            if (coin) {
                if (coin->isOffScreen() || coin->isCollected()) {
                    removeEntity = true;
                } else if (player->getGlobalBounds().intersects(coin->getGlobalBounds())) {
                    coin->collect();
                    coins++;
                    saveGame();
                    removeEntity = true;
                }
            }
            
            if (removeEntity) {
                it = gameEntities.erase(it);
            } else {
                ++it;
            }
        }
    }

    void centerText(sf::Text& text, float x, float y) {
        sf::FloatRect textRect = text.getLocalBounds();
        text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top  + textRect.height/2.0f);
        text.setPosition(sf::Vector2f(x, y));
    }

    void render() {
        window.clear(sf::Color(20, 20, 30)); // Deep night sky

        for (const auto& s : stars) s.draw(window);
        for (const auto& m : mountains) m.draw(window);

        sf::RectangleShape groundLine(sf::Vector2f(screenWidth, 5.0f));
        groundLine.setPosition(0, groundY + 50.0f); 
        groundLine.setFillColor(sf::Color::White);
        window.draw(groundLine);

        if (currentState == GameState::Playing || currentState == GameState::GameOver) {
            player->draw(window);
            for (const auto& ent : gameEntities) {
                ent->draw(window);
            }
            
            scoreText.setString("Score: " + std::to_string(score) + " | Coins: " + std::to_string(coins) + "\nHigh Score: " + std::to_string(highScore));
            window.draw(scoreText);
        }

        if (currentState == GameState::Menu) {
            titleText.setString("STICKMAN SURVIVOR");
            titleText.setFillColor(sf::Color::White);
            centerText(titleText, screenWidth/2.0f, 150.0f);
            
            instructionText.setString("Press SPACE to Start\nPress S to enter STORE");
            centerText(instructionText, screenWidth/2.0f, 300.0f);
            
            window.draw(titleText);
            window.draw(instructionText);
        }
        else if (currentState == GameState::Store) {
            titleText.setString("CHARACTER STORE");
            titleText.setFillColor(sf::Color::Yellow);
            centerText(titleText, screenWidth/2.0f, 100.0f);
            
            storeText.setString(
                "Your Coins: " + std::to_string(coins) + "\n\n" +
                "1: Green Stickman (Free/Default)\n" +
                "2: Blue Stickman (10 Coins) " + (ownsBlue ? "[OWNED]" : "") + "\n" +
                "3: Purple Stickman (20 Coins) " + (ownsPurple ? "[OWNED]" : "") + "\n\n" +
                "Press the number key to buy/equip!\n" +
                "Press ESC to return to Menu"
            );
            centerText(storeText, screenWidth/2.0f, 300.0f);

            window.draw(titleText);
            window.draw(storeText);
        }
        else if (currentState == GameState::GameOver) {
            titleText.setString("GAME OVER");
            titleText.setFillColor(sf::Color::Red);
            centerText(titleText, screenWidth/2.0f, 150.0f);

            instructionText.setString("Press SPACE to Restart\nPress M to return to Menu");
            centerText(instructionText, screenWidth/2.0f, 300.0f);

            window.draw(titleText);
            window.draw(instructionText);
        }

        window.display();
    }
};

int main() {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    Game game;
    game.run();
    return 0;
}
