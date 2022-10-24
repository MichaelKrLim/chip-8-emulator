#ifndef GAMESTATE_H
#define GAMESTATE_H

struct Gamestate
{
	sf::Uint8* pixels = new sf::Uint8[64 * 32 * 4];
	std::bitset<64 * 32> screen{};

	std::array<std::uint8_t, 4096> memory{};
	std::array<std::uint8_t, 16> registers{};

	std::vector<int> stack;

	int program_counter{ 0x200 };
	int screen_width = 1280;
	int screen_length = 960;

	uint8_t delay_timer{};
	uint8_t sound_timer{};
	uint16_t I{};
};

#endif