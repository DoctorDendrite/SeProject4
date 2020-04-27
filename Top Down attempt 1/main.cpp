#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <Windows.h>

using namespace sf;

static const float PI = 3.141592654f;

class Bullet
{
public:
	RectangleShape shape;
	Vector2f currVelocity;
	sf::Texture bulletFile;

	Bullet(float height = 4.f)
		: currVelocity(0.f, 0.f)
	{			
		this->shape.setOrigin(Vector2f(this->shape.getPosition().x + (height * 3 / 2), this->shape.getPosition().y + (height / 2)));
		this->shape.setSize(Vector2f(height * 3, height));
		//this->shape.setFillColor(Color::Yellow);
	}	
};


// Universal gun data
enum GUNS { BERETTA, GLOCK, REMINGTON, AA12, M14, AK47, NUM_OF_GUNS };

struct gunStats
{
	// Universal stats
	std::string name;
	float damage;
	float currRPM, muzzVelocity;
	bool isSemiAuto;
	float spreadDegrees;
	int magCapacity;
	float reloadTime;
	
	// Shotgun-specific
	bool isShotgun;
	size_t numPellets;	

	// Auto calculate
	float currFireTime = 60.f / currRPM;	
};


void prepGuns(std::vector<gunStats> &listOfGunStats);

std::string getKillCountStr(int totalKills)
{
	int numDigits = 0;
	std::string output;
	int n = totalKills;

	while (n != 0)
	{
		n = n / 10;
		numDigits++;
	}

	numDigits = 4 - numDigits;

	for (int i = 0; i < numDigits; i++)
	{
		output.append("0");
	}
	output.append(std::to_string(totalKills));

	return output;
}


int main()
{
	// Temp rand (use better formula)
	srand(time(NULL));
	
	bool isFullscreen = true;
	
	RenderWindow window(VideoMode(VideoMode::getDesktopMode().width, VideoMode::getDesktopMode().height), "360 Shooter!", Style::Default | Style::Fullscreen);
	window.setFramerateLimit(60);
	window.setVerticalSyncEnabled(true);
	window.setMouseCursorVisible(false); 
	
	// Crosshair
	sf::Texture crosshairTexture;
	sf::RectangleShape crosshair(Vector2f(31.f, 31.f));
	crosshair.setOrigin((crosshair.getSize() + Vector2f(1.f, 1.f)) / 2.f);
	crosshairTexture.loadFromFile("textures/crosshair.png");
	crosshair.setTexture(&crosshairTexture);

	

	// PLAYER variables

	// Drawing
	RectangleShape player(Vector2f(31.f, 31.f));
	Texture playerTexture;
	playerTexture.loadFromFile("textures/player.png");	// UPDATE, just a debug
	player.setTexture(&playerTexture);
	//player.setFillColor(Color::Blue);	
	player.setSize(Vector2f(100.f, 100.f));// Player char circle is colored white
	player.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f);		// Initially, put player in the center
	player.setOrigin(player.getSize() / 2.f);
	Vector2f playerVelocity(0.f, 0.f);

	// Player data
	float playerSpeed;
	const float WALK_SPEED = 160.f;
	float currSpeed = WALK_SPEED;
	const float distMuzzFromChar = 29.f;
	bool faceRight;
	float playerHealth = 100.f;
	bool isAlive = true;
	
	// Controls
	std::vector<Keyboard::Key> myKeyBinds = { Keyboard::W, Keyboard::S, Keyboard::A, Keyboard::D, Keyboard::R };
	enum CONTROLS { MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT, RELOAD };
	std::vector<std::string> ControlNames = { "Move Up", "Move Down", "Move Left", "Move Right", "Reload" };
	
	// Init test

	// Bullet variables
	Bullet aBullet(4.f);
	std::vector<Bullet> bullets;

	aBullet.bulletFile.loadFromFile("textures/b1.png");
	aBullet.shape.setTexture(&aBullet.bulletFile);

	
	// Enemy variables
	RectangleShape enemy;
	Texture enemyTexture;
	enemyTexture.loadFromFile("textures/BadGuy.png");	// UPDATE, just a debug
	enemy.setTexture(&enemyTexture);
	enemy.setFillColor(Color::Red);
	enemy.setSize(Vector2f(100.f, 100.f));

	float enemySpeed = 12.f;
	float enemyDMG = 0.15f;

	std::vector<std::pair<RectangleShape, float>> enemies;
	int spawnCounter = 20;

	int totalKills = 0;

	int roundNum = 1;
	int numZombies = roundNum * 2 + 10;

	// Vectors
	Vector2f playerCenter, mousePosWindow, aimDir, aimDirNorm, enemyToPlayer;


	// Timing related
	Clock clockGameTime, clockFT, clockRoF, clockReload;	// clockFT is for measuring FRAMETIME, clockROF is to measure RATE OF FIRE
	Clock clockMuzzFlash, clockImpact;
	float deltaTime;	// deltaTime is frametime;


	// Gun related
	int currGun;
	gunStats currGunStats;
	std::vector<gunStats> listOfGunStats;
	prepGuns(listOfGunStats);
	
	// enum GUNS { BERETTA, GLOCK, REMINGTON, AA12, M14, AK47, NUM_OF_GUNS };
	currGun = BERETTA;
	currGunStats = listOfGunStats[currGun];
	bool triggerHeld = false;

	bool isReloading = false, manualReload = false;
	std::cout << ">> Using: " << currGunStats.name << "\n";
	
	bool switchedGun = true;


	// Ammo count (bullet icons)
	std::vector<int> allAmmoCounts(listOfGunStats.size());
	for (int i = 0; i < listOfGunStats.size(); i++)
		allAmmoCounts[i] = listOfGunStats[i].magCapacity;

	std::cout << "allAmmoCounts[currGun]: " << allAmmoCounts[currGun] << "\n";

	RectangleShape bulletIcon;
	Texture ammoTexture;
	ammoTexture.loadFromFile("textures/pistol_round.png");
	bulletIcon.setTexture(&ammoTexture);
	bulletIcon.setFillColor(Color::Yellow);
	bulletIcon.setSize(Vector2f(32.f, 64.f));
	
	std::vector<RectangleShape> ammoCount;
	for (int i = 0; i < 40; i++)
	{
		bulletIcon.setPosition(Vector2f((VideoMode::getDesktopMode().width / 28.f) + i * (bulletIcon.getSize().x - 2), VideoMode::getDesktopMode().height * 9.f / 10.f));
		ammoCount.push_back(bulletIcon);
	}

	RectangleShape reloadIcon;
	Texture reloadTexture;
	reloadTexture.loadFromFile("textures/reloading_icon.png");
	reloadIcon.setTexture(&reloadTexture);
	//reloadIcon.setFillColor(Color::White);
	reloadIcon.setOutlineColor(Color::White);
	reloadIcon.setSize(Vector2f(384.f, 64.f));
	reloadIcon.setPosition((VideoMode::getDesktopMode().width / 28.f), VideoMode::getDesktopMode().height * 9.f / 10.f);

	RectangleShape reloadBar;
	reloadBar.setFillColor(Color::Red);
	reloadBar.setSize(Vector2f(reloadIcon.getSize().x + 20, reloadIcon.getSize().y));
	reloadBar.setPosition(Vector2f(reloadIcon.getPosition().x - 10, reloadIcon.getPosition().y - 3));
		

	RectangleShape impactMark;
	Texture impactTexture;
	impactTexture.loadFromFile("textures/impact.png");
	impactMark.setTexture(&impactTexture);
	impactMark.setSize(Vector2f(12.f, 8.f));
	impactMark.setScale(Vector2f(2.f, 2.f));


	RectangleShape muzzleFlash;
	Texture flashTexture;
	flashTexture.loadFromFile("textures/muzzleFlash.png");
	muzzleFlash.setTexture(&flashTexture);
	muzzleFlash.setSize(Vector2f(12.f, 8.f));
	muzzleFlash.setScale(Vector2f(1.5f, 1.5f));


	// Sounds	
	std::vector<SoundBuffer> all_gunshotBuffers(NUM_OF_GUNS);
	all_gunshotBuffers[BERETTA].loadFromFile("sounds/beretta_gunshot.wav");
	all_gunshotBuffers[GLOCK].loadFromFile("sounds/glock_gunshot.wav");
	all_gunshotBuffers[REMINGTON].loadFromFile("sounds/remington_gunshot.wav");
	all_gunshotBuffers[AA12].loadFromFile("sounds/aa12_gunshot.wav");
	all_gunshotBuffers[M14].loadFromFile("sounds/m14_gunshot.wav");
	all_gunshotBuffers[AK47].loadFromFile("sounds/ak47_gunshot.wav");



	std::vector<Sound> allGunShots(NUM_OF_GUNS);
	for (int i = 0; i < NUM_OF_GUNS; i++)
	{
		allGunShots[i].setBuffer(all_gunshotBuffers[i]);
		allGunShots[i].setVolume(50.f);
	}
	
	
	std::vector<SoundBuffer> all_reloadBuffers(NUM_OF_GUNS);
	all_reloadBuffers[BERETTA].loadFromFile("sounds/beretta_reload.wav");
	all_reloadBuffers[GLOCK].loadFromFile("sounds/glock_reload.wav");
	all_reloadBuffers[REMINGTON].loadFromFile("sounds/remington_reload.wav");
	all_reloadBuffers[AA12].loadFromFile("sounds/aa12_reload.wav");
	all_reloadBuffers[M14].loadFromFile("sounds/m14_reload.wav");
	all_reloadBuffers[AK47].loadFromFile("sounds/ak47_reload.wav");


	std::vector<Sound> allGunReloads(NUM_OF_GUNS);
	for (int i = 0; i < NUM_OF_GUNS; i++)
		allGunReloads[i].setBuffer(all_reloadBuffers[i]);



	// Kill counter
	RectangleShape killCounterIcon;
	Texture killCounterTexture;
	killCounterTexture.loadFromFile("textures/killcounter_icon.png");
	killCounterIcon.setTexture(&killCounterTexture);
	killCounterIcon.setSize(Vector2f(64.f, 28.f));
	killCounterIcon.setScale(4.5f, 4.1f);
	killCounterIcon.setPosition(Vector2f(1600.f, 28.f));
	
	Font chiller_font;
	chiller_font.loadFromFile("fonts/chiller.ttf");
	Text killCount;
	killCount.setFont(chiller_font);
	killCount.setString("0000");
	killCount.setFillColor(Color::White);
	killCount.setPosition(killCounterIcon.getPosition() + Vector2f(124.f, 40.f));
	killCount.setCharacterSize(48);
	killCount.setOutlineColor(Color::White);
	killCount.setOutlineThickness(0.5f);


	// Health bar
	RectangleShape healthBar;
	healthBar.setFillColor(Color::Red);
	healthBar.setSize(Vector2f(400.f, 40.f));
	healthBar.setPosition(100.f, 80.f);


	Text health;
	health.setFont(chiller_font);
	health.setPosition(healthBar.getPosition() + Vector2f(20.f, 0.f));
	health.setFillColor(Color::White);
	health.setOutlineColor(Color::Black);
	health.setOutlineThickness(1.f);
	health.setCharacterSize(32);
	health.setString("Health: " + std::to_string(int(playerHealth)));


	Text youDied;
	youDied.setFont(chiller_font);
	youDied.setPosition(600.f, 400.f);
	youDied.setFillColor(Color::White);
	youDied.setOutlineColor(Color::Black);
	youDied.setOutlineThickness(2.f);
	youDied.setCharacterSize(250);
	youDied.setString("You Died.");


	// Gun name
	Text gunName;
	gunName.setFont(chiller_font);
	gunName.setString(currGunStats.name);
	gunName.setCharacterSize(64);
	gunName.setPosition(reloadBar.getPosition() + Vector2f(22.f, -82.f));
	gunName.setFillColor(Color::White);
	gunName.setOutlineColor(Color::Black);
	gunName.setOutlineThickness(2.f);

	
	// Background
	Sprite bgTile1;
	Texture tile1;
	tile1.loadFromFile("textures/bg_tile1.png");
	tile1.setRepeated(true);
	bgTile1.setTexture(tile1);
	bgTile1.setTextureRect({ 0, 0, 1920, 1080 });
	/*bgTile1.setPosition(0.f, 0.f);
	bgTile1.setSize(Vector2f(VideoMode::getDesktopMode().width, VideoMode::getDesktopMode().height));*/


	// Round Counter
	Text roundCount;
	roundCount.setFont(chiller_font);
	roundCount.setString("Round " + std::to_string(roundNum));
	roundCount.setCharacterSize(120);
	roundCount.setFillColor(Color::White);
	roundCount.setOutlineColor(Color::Black);
	roundCount.setOutlineThickness(1.f);
	roundCount.setPosition(Vector2f(VideoMode::getDesktopMode().width * 11.25f / 12.f, VideoMode::getDesktopMode().height) * 6.7f / 8.f);


	// Round Timer
	int lengthOfRound = 15.f;
	RectangleShape roundTimerBar;
	roundTimerBar.setFillColor(Color::Red);
	roundTimerBar.setOutlineColor(Color::Black);
	roundTimerBar.setOutlineThickness(2.f);
	roundTimerBar.setSize(Vector2f(384.f, 24.f));
	roundTimerBar.setPosition(roundCount.getPosition() + Vector2f(-32.f, 140));

	
	// Start music
	Music bgm1;
	bgm1.openFromFile("sounds/Clayton_Spooky_Comp.wav");
	bgm1.setLoop(true);
	bool bgmPlaying = true;


	clockGameTime.restart();

	while (window.isOpen())
	{
		deltaTime = clockFT.restart().asSeconds();
		
		Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
				case Event::Closed:
					window.close();
					break;

				case Event::KeyReleased:
					if (event.key.code == Keyboard::Escape)
					{
						if (isFullscreen)
						{
							window.create(sf::VideoMode(VideoMode::getDesktopMode().width, VideoMode::getDesktopMode().height), "360 Shooter!", sf::Style::Default);
							window.setMouseCursorVisible(true);
							isFullscreen = false;
						}
						else
						{
							window.create(sf::VideoMode(VideoMode::getDesktopMode().width, VideoMode::getDesktopMode().height), "360 Shooter!", sf::Style::Fullscreen);
							window.setMouseCursorVisible(false);
							isFullscreen = true;
						}
					}
					else if (event.key.code == Keyboard::M)
					{
						if (bgmPlaying)
						{
							bgmPlaying = false;
							bgm1.pause();							
						}							
						else
						{
							bgmPlaying = true;
							bgm1.play();
						}							
					}
					break;

					// Every time we switch weapons, reset the current allAmmoCounts[currGun] count
				case Event::KeyPressed:
					switchedGun = true;
					switch (event.key.code)
					{
					case Keyboard::Num1:
						currGun = BERETTA;
						ammoTexture.loadFromFile("textures/pistol_round.png");
						break;

					case Keyboard::Num2:
						currGun = GLOCK;
						ammoTexture.loadFromFile("textures/pistol_round.png");
						break;

					case Keyboard::Num3:
						currGun = REMINGTON;
						ammoTexture.loadFromFile("textures/shotgun_round.png");
						break;

					case Keyboard::Num4:
						currGun = AA12;
						ammoTexture.loadFromFile("textures/shotgun_round.png");
						break;

					case Keyboard::Num5:
						currGun = M14;
						ammoTexture.loadFromFile("textures/rifle_round.png");
						break;

					case Keyboard::Num6:
						currGun = AK47;
						ammoTexture.loadFromFile("textures/rifle_round.png");
						break;

					default:
						switchedGun = false;
						break;
					}

					if (switchedGun)
					{						
						currGunStats = listOfGunStats[currGun];
						allAmmoCounts[currGun] = allAmmoCounts[currGun];
						gunName.setString(currGunStats.name);
						isReloading = false;
						clockReload.restart();

						system("cls");
						std::cout << ">> Switched to: " << currGunStats.name << "\n";
						std::cout << "allAmmoCounts[currGun]: " << allAmmoCounts[currGun] << "\n";
					}
					
					break;

				case Event::Resized:
					break;
			}
		}		
		
		crosshair.setPosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
		

		// GAME LOGIC
		// Update
		playerCenter = Vector2f(player.getPosition().x , player.getPosition().y);
		mousePosWindow = Vector2f(Mouse::getPosition(window));
		aimDir = mousePosWindow - playerCenter;
		aimDirNorm = aimDir / sqrt(pow(aimDir.x, 2) + pow(aimDir.y, 2));

		if(isAlive)
			player.setRotation(atan2f(aimDirNorm.y, aimDirNorm.x) * 180.f / PI);
		
		// ~~~ DEBUG
		//std::cout << aimDirNorm.x << " " << aimDirNorm.y << "\t\tdT = " << deltaTime << std::endl;


		// Only allow movement if player is alive
		if (isAlive)
		{
			// Movement
			// Detect diagonal movements and counteract Pythagorean multiplier if necessary
			if ((Keyboard::isKeyPressed(myKeyBinds[MOVE_LEFT]) || Keyboard::isKeyPressed(myKeyBinds[MOVE_RIGHT]))
				&& (Keyboard::isKeyPressed(myKeyBinds[MOVE_UP]) || Keyboard::isKeyPressed(myKeyBinds[MOVE_DOWN])))
				playerSpeed = currSpeed * sinf(45.f * PI / 180);
			else
				playerSpeed = currSpeed;

			// Reset the velocity
			playerVelocity = Vector2f(0.f, 0.f);



			// Update the current player velocity
			if (Keyboard::isKeyPressed(myKeyBinds[MOVE_LEFT]))
				playerVelocity.x += -playerSpeed * deltaTime;
			if (Keyboard::isKeyPressed(myKeyBinds[MOVE_RIGHT]))
				playerVelocity.x += playerSpeed * deltaTime;
			if (Keyboard::isKeyPressed(myKeyBinds[MOVE_UP]))
				playerVelocity.y += -playerSpeed * deltaTime;
			if (Keyboard::isKeyPressed(myKeyBinds[MOVE_DOWN]))
				playerVelocity.y += playerSpeed * deltaTime;

			// Detect if player wants to and is allowed to reload (less than full mag)
			if (Keyboard::isKeyPressed(myKeyBinds[RELOAD]) && allAmmoCounts[currGun] < currGunStats.magCapacity)
				allAmmoCounts[currGun] = 0;


			// Move the player to their velocity
			player.move(playerVelocity);

			if (crosshair.getPosition().x < player.getPosition().x)
			{
				player.setScale(1, -1);
				faceRight = false;
			}
			else
			{
				player.setScale(1, 1);
				faceRight = true;
			}


			// Round update, every lengthOfRound number of seconds (default 15 seconds)
			if (clockGameTime.getElapsedTime().asSeconds() >= lengthOfRound)
			{
				// Increment the round counter
				roundNum++;

				// Slightly increase movement speed
				enemySpeed += 0.2f;

				// Increase number of zombies on screen
				numZombies = roundNum * 2 + 10;

				// Display the new round count
				roundCount.setString("Round " + std::to_string(roundNum));

				// Restart the round clock timer
				clockGameTime.restart();
			}

			// Enemy update
			if (spawnCounter < numZombies)
				spawnCounter++;

			// Spawn outside the map
			if (spawnCounter >= numZombies && enemies.size() <= numZombies + 5 && isAlive)
			{
				// Pick one of the four corners
				switch (rand() % 4)
				{
					// Left side
				case 0:
					enemy.setPosition(Vector2f(-30.f, rand() % window.getSize().y));
					break;

					// Right side
				case 1:
					enemy.setPosition(Vector2f(window.getSize().x + 30.f, rand() % window.getSize().y));
					break;

					// Top side
				case 2:
					enemy.setPosition(Vector2f(rand() % window.getSize().x, -30.f));
					break;

					// Bot side
				case 3:
					enemy.setPosition(Vector2f(rand() % window.getSize().x, window.getSize().y + 30.f));
					break;
				}

				enemies.push_back(std::make_pair(RectangleShape(enemy), 100.f));

				spawnCounter = 0;
			}



			// Make every enemy move towards the player
			for (size_t i = 0; i < enemies.size(); i++)
			{
				// Find the direction of the player
				enemyToPlayer = playerCenter - enemies[i].first.getPosition();

				// Normalize
				enemyToPlayer /= sqrt(pow(enemyToPlayer.x, 2) + pow(enemyToPlayer.y, 2));

				enemies[i].first.move(enemyToPlayer * deltaTime * enemySpeed);
			}


			// Shooting - check if the trigger WAS JUST held down before we check for shooting
			if (!Mouse::isButtonPressed(Mouse::Left))
				triggerHeld = false;

			// If we aren't in the specific case that it is a semi-auto, and we haven't held the trigger down, then WE CAN SHOOTTTT
			// But only if we have rounds left in the current magazine
			if (!(currGunStats.isSemiAuto && triggerHeld) && allAmmoCounts[currGun] > 0)
			{
				// If the trigger (LMB) is pressed and there's been enough elapsed time since the previous shot
				if (clockRoF.getElapsedTime().asSeconds() >= currGunStats.currFireTime && Mouse::isButtonPressed(Mouse::Left))
				{
					// THEN WE ARE FIRING

					Vector2f currVector;
					float currAngle, currAngleRad;

					// If it fires one bullet at a time (aka not a shotgun)
					if (!currGunStats.isShotgun)
					{
						// Pick a random spread for the bullet
						currAngle = rand() % (int(currGunStats.spreadDegrees * 100) / 100) - int(currGunStats.spreadDegrees * 50) / 100 - 0.5f;
						//printf("currAngle: %f\n", currAngle);
						currAngleRad = currAngle * PI / 180.f;

						// Rotate vector of bullet direction
						currVector.x = aimDirNorm.x * cosf(currAngleRad) - aimDirNorm.y * sinf(currAngleRad);
						currVector.y = aimDirNorm.x * sinf(currAngleRad) + aimDirNorm.y * cosf(currAngleRad);

						// Initially set the position of the bullet (and muzzle flash)
						aBullet.shape.setPosition(playerCenter);
						aBullet.shape.setRotation(atan2f(currVector.y, currVector.x) * 180.f / PI);
						aBullet.shape.move(aimDirNorm * distMuzzFromChar);
						aBullet.currVelocity = currVector * currGunStats.muzzVelocity * deltaTime + playerVelocity;
						bullets.push_back(Bullet(aBullet));
						// ADD ROTATION OF BULLET TO POINT IN SPECIFIC DIRECTION BC WE'LL REPLACE IT WITH A RECTANGLE SHAPE 

						clockRoF.restart().Zero;
						triggerHeld = true;
					}
					// Otherwise, if it fires multiple pellets like a shotgun (rework this boolean to an enum later to add lasers and other weird bullets)
					else
					{
						float degBetweenPellets = currGunStats.spreadDegrees / (currGunStats.numPellets - 1);

						currAngle = -currGunStats.spreadDegrees / 2.f;
						currAngleRad = currAngle * PI / 180.f;

						for (size_t currPellet = 0; currPellet < currGunStats.numPellets; currPellet++)
						{
							// Rotate vector formula
							currVector.x = aimDirNorm.x * cosf(currAngleRad) - aimDirNorm.y * sinf(currAngleRad);
							currVector.y = aimDirNorm.x * sinf(currAngleRad) + aimDirNorm.y * cosf(currAngleRad);

							aBullet.shape.setPosition(playerCenter);
							aBullet.shape.setRotation(atan2f(aimDirNorm.y, aimDirNorm.x) * 180.f / PI);
							aBullet.shape.move(aimDirNorm * distMuzzFromChar);
							aBullet.currVelocity = currVector * currGunStats.muzzVelocity * deltaTime + playerVelocity;
							bullets.push_back(Bullet(aBullet));

							// fix later?
							if (currPellet == currGunStats.numPellets - 1)
								currAngle = currGunStats.spreadDegrees / 2.f;
							else
								currAngle += degBetweenPellets + (rand() % int(currGunStats.spreadDegrees * 10) / 70 - (int)(currGunStats.spreadDegrees * 10 / 70));

							currAngleRad = currAngle * PI / 180.f;
							clockRoF.restart().Zero;
							triggerHeld = true;

						}	// End for-loop for each shotgun pellet
					}	// End if-else statement for shooting

					muzzleFlash.setRotation(aBullet.shape.getRotation() + 90.f);

					// A really bad attempt to align muzzle flash
					if (faceRight)
					{
						muzzleFlash.setPosition(aBullet.shape.getPosition() - (muzzleFlash.getSize() / 1.25f));
						muzzleFlash.move(aimDirNorm * (distMuzzFromChar - 1.f));
					}
					else
					{
						muzzleFlash.setPosition(aBullet.shape.getPosition() + (muzzleFlash.getSize() / 2.f));
						muzzleFlash.move(aimDirNorm * (distMuzzFromChar - 4.f));
					}

					clockMuzzFlash.restart();

					// We need to decrement the number of rounds left
					allAmmoCounts[currGun]--;

					// Play the corresponding gunshot sound
					allGunShots[currGun].play();


					// Debug
					std::cout << "allAmmoCounts[currGun]: " << allAmmoCounts[currGun] << "\n";

				}	// End firetime if-statement
			}	// End semi-auto/empty mag check
			// If we have run out of rounds, set it to we are reloading
			else if (allAmmoCounts[currGun] <= 0)
			{
				// If we aren't reloading yet AKA WE *JUST* emptied the mag,
				// Then we have to change to recognize we now ARE reloading and start the timer
				if (!isReloading)
				{
					// Say we are reloading
					isReloading = true;

					// Start the timer
					clockReload.restart();

					// Play the reload sound
					allGunReloads[currGun].play();

					// Debug
					std::cout << ">> RELOADING: [" << currGunStats.name << "]" << "\n";
				}
				// If we already are reloading, then check to see if we are done
				else
				{
					if (clockReload.getElapsedTime().asSeconds() >= currGunStats.reloadTime)
					{
						// We're no longer reloading
						isReloading = false;

						// Reset the number of rounds left in the mag to the max
						allAmmoCounts[currGun] = currGunStats.magCapacity;

						// In case it's semi auto require the trigger to be pulled again before you can fire (else it immediately fires a shot)
						triggerHeld = true;		// full auto guns are ok being able to fire immediately after reloading

						// Debug
						std::cout << ">> Finished reloading.\n";
						std::cout << "allAmmoCounts[currGun]: " << allAmmoCounts[currGun] << "\n";
					}
				}
			}

			// Enemy hurting player
			for (size_t i = 0; i < enemies.size(); i++)
			{
				if (enemies[i].first.getGlobalBounds().intersects(player.getGlobalBounds()))
				{
					if (playerHealth > 0)
					{
						playerHealth -= enemyDMG;
						health.setString("Health: " + std::to_string(int(playerHealth)));
					}
					else
						isAlive = false;
				}
			}



			// Update bullets
			for (size_t i = 0; i < bullets.size(); i++)
			{
				// Update bullet position via its respective velocity
				bullets[i].shape.move(bullets[i].currVelocity);

				// If bullet is out of bounds (window)
				if (bullets[i].shape.getPosition().x < 0 - aBullet.shape.getSize().x * 2 || bullets[i].shape.getPosition().x > window.getSize().x + aBullet.shape.getSize().x * 2
					|| bullets[i].shape.getPosition().y < 0 - aBullet.shape.getSize().y * 2 || bullets[i].shape.getPosition().y > window.getSize().y + aBullet.shape.getSize().y * 2)
				{
					// Then delete them to free memory
					bullets.erase(bullets.begin() + i);
				}
				else
				{
					// Enemy collision, for every single enemy
					for (size_t k = 0; k < enemies.size(); k++)
					{
						// If the bullet comes into contact with an enemy
						if (bullets[i].shape.getGlobalBounds().intersects(enemies[k].first.getGlobalBounds()))
						{
							// Decrease the health of this particular enemy
							enemies[k].second -= currGunStats.damage;

							// Put impact marker
							impactMark.setPosition(bullets[i].shape.getPosition());

							impactMark.setRotation(bullets[i].shape.getRotation() + 90.f);

							clockImpact.restart();

							// Delete the bullet on contact
							bullets.erase(bullets.begin() + i);


							// Debug
							//std::cout << "New health of enemy #" << k << ": " << enemies[k].second << "\n";

							// Then check if killed (health went <= 0)
							if (enemies[k].second <= 0.f)
							{
								// Delete the enemy
								enemies.erase(enemies.begin() + k);

								// Increment number of kills
								totalKills++;

								// Update display value
								killCount.setString(getKillCountStr(totalKills));
							}
							break;
						}
					}
				}
			}
		}
		// FINISHED GAME LOGIC

		// DRAW EVERYTHING (window)
		// Clear first
		window.clear();

		// Draw background
		window.draw(bgTile1);

		// Draw enemies
		for (size_t i = 0; i < enemies.size(); i++)
			window.draw(enemies[i].first);

		// Draw every bullet
		for (size_t i = 0; i < bullets.size(); i++)
			window.draw(bullets[i].shape);

		if(clockImpact.getElapsedTime().asSeconds() <= 0.02f)
			window.draw(impactMark);
		

		// Then draw player (on top of bullets)
		window.draw(player);
		

		if (isReloading)
		{
			// Draw the bar
			reloadBar.setScale(clockReload.getElapsedTime().asSeconds() / currGunStats.reloadTime, 1.f);
			window.draw(reloadBar);

			// Draw the reloading icon on top
			window.draw(reloadIcon);
		}
			
		
		// Finally, on the very top, HUD: ammo count. Draw only the amount of bullets required
		for (int i = 0; i < allAmmoCounts[currGun]; i++)
			window.draw(ammoCount[i]);

		if (clockMuzzFlash.getElapsedTime().asSeconds() <= 0.06f)
			window.draw(muzzleFlash);
			

		window.draw(gunName);
		window.draw(killCounterIcon);
		window.draw(killCount);
		window.draw(roundCount);

		// Show health bar
		healthBar.setScale(playerHealth / 100.f, 1.f);
		window.draw(healthBar);
		window.draw(health);

		// Show round timer
		roundTimerBar.setScale(1.f - (clockGameTime.getElapsedTime().asSeconds() / float(lengthOfRound)), 1.f);
		window.draw(roundTimerBar);

		window.draw(crosshair);

		// If player is dead, print you died
		if (!isAlive)
			window.draw(youDied);
		

		window.display();
	}


	return 0;
}


void prepGuns(std::vector<gunStats> &listOfGunStats)
{
	listOfGunStats.clear();
	
	listOfGunStats.resize(NUM_OF_GUNS);

	// enum GUNS { BERETTA, GLOCK, REMINGTON, AA12, M14, AK47, NUM_OF_GUNS };

	//							 Name			Damage	RoF		MuzzVel	semi?	spread	mag	reload	shotty?, #pellets
	listOfGunStats[BERETTA] =	{"Beretta",		25.f,	630.f,	1000.f,	true,	10.f,	15,	1.5f,	false,	0	};
	listOfGunStats[GLOCK] =		{"Glock",		20.f,	1100.f,	950.f,	false,	15.f,	33,	1.7f,	false,	0	};
	listOfGunStats[REMINGTON] = {"Remington",	40.f,	75.f,	1200.f,	true,	10.f,	8,	3.4f,	true,	7,  };
	listOfGunStats[AA12] =		{"AA-12",		20.f,	300.f,	1000.f,	false,	20.f,	32,	3.3f,	true,	6,  };
	listOfGunStats[M14] =		{"M14",			60.f,	340.f,	2300.f,	true,	3.5f,	20,	2.6f,	false,	0	};
	listOfGunStats[AK47] =		{"AK-47",		40.f,	600.f,	2600.f,	false,	6.0f,	30,	2.55f,	false,	0	};


	/*
	struct gunStats
	{
		// Universal stats		
		std::string name;
		float damage;
		float currRPM, muzzVelocity;
		bool isSemiAuto;
		float spreadDegrees;
		int magCapacity;
		float reloadTime;
	
		// Shotgun-specific
		bool isShotgun;
		size_t numPellets;	

		// Auto calculate
		float currFireTime = 60.f / currRPM;	
	};
	*/
}
