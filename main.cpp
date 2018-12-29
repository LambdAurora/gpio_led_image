/*
 * Copyright © 2018 "LambdAurora" <aurora42lambda@gmail.com>
 * 
 * This file is part of gpio_led_image.
 * 
 * Licensed under the MIT license. For more information, 
 * see the LICENSE file.  
 */

#include <lambdacommon/graphics/color.h>
#include <lambdacommon/system/system.h>
#include <wiringPi.h>
#include <softPwm.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <fstream>
#include <cmath>

#define CONFIG_FILE_PATH "/etc/gpio_led_image.txt"

namespace term = lambdacommon::terminal;
namespace fs = lambdacommon::fs;
namespace sys = lambdacommon::system;
using lambdacommon::system::sleep;
typedef std::vector<term::TermFormatting> tformats_t;

class Context
{
private:
	int _red_pin, _green_pin, _blue_pin;

public:
	Context(int redpin = 0, int greenpin = 2, int bluepin = 3) : _red_pin(redpin), _green_pin(greenpin), _blue_pin(bluepin)
	{}

	void load_config_file()
	{
		auto config_file = fs::FilePath(std::string(CONFIG_FILE_PATH));
		if (!config_file.exists())
			create_config_file();
		std::ifstream config_in;
		config_in.open(config_file.to_string());
		if (config_in.is_open()) {
			std::string line;
			int linec = 0;
			while (getline(config_in, line)) {
				if (!lambdacommon::lstring::starts_with(line, "#")) {
					switch (linec) {
						case 0:
							_red_pin = lambdacommon::lstring::parse_int(line);
							break;
						case 1:
							_green_pin = lambdacommon::lstring::parse_int(line);
							break;
						case 2:
							_blue_pin = lambdacommon::lstring::parse_int(line);
							break;
						default:
							std::cout << "[WARNING] Unneeded line detected!" << std::endl;
							break;
					}
					linec++;
				}
			}
			config_in.close();
		}
	}

	void create_config_file()
	{
		auto config_file = fs::FilePath(CONFIG_FILE_PATH);
		if (config_file.exists())
			return;
		auto parent_dir = config_file.get_parent();
		if (!parent_dir.exists())
			if(!parent_dir.mkdir(true)) {
				std::cerr << term::RED << "Error: " << term::LIGHT_RED << "cannot create the folder '" << parent_dir.to_string() << "'!" << term::RESET << std::endl;
				return;
			}
		std::ofstream config_out;
		config_out.open(config_file.to_string());
		if (config_out.is_open()) {
			config_out << "# gpio_led_image.txt" << std::endl;
			config_out << "# Represents the configuration file of gpio_led_image, a software made just for fun with RGB leds." << std::endl;
			config_out << "# This file is made of 3 uncommented lines: they are the RGB pins of the led, the first one is the RED pin, the second one is the GREEN pin and the last one is the BLUE pin." << std::endl;
			config_out << "0" << std::endl;
			config_out << "2" << std::endl;
			config_out << "3" << std::endl;
			config_out.close();
		}
	}

	int get_red_pin() const
	{
		return _red_pin;
	}

	int get_green_pin() const
	{
		return _green_pin;
	}

	int get_blue_pin() const
	{
		return _blue_pin;
	}
};

void setup_gpio(const Context &context);

/*!
 * Sets the led off.
 *
 * @param context The context.
 */ 
void set_led_off(const Context &context);

void hello_world_led(const Context &context);

/*!
 * Gets the color value of the specified pixel of the image.
 *
 * @param image The image.
 * @param width The image width.
 * @param channels The number of channels of the image (usually 4).
 * @param x The X coordinate of the pixel.
 * @param y The Y coordinate of the pixel.
 *
 * @return The color of the pixel.
 */ 
lambdacommon::Color get_pixel(stbi_uc *image, int width, int channels, size_t x, size_t y);

int main(int argc, char **argv)
{
	term::setup();

std::cout << "Starting GPIO " << tformats_t(term::BOLD, term::RED) << 'R' << tformats_t(term::BOLD, term::GREEN) << 'G' << tformats_t(term::BOLD, term::BLUE) << 'B' << term::RESET << " led image viewer " << term::MAGENTA << "(just for fun)" << term::RESET << "..." << std::endl;

	if (!sys::is_root()) {
		std::cout << term::RED << "Error: " << term::LIGHT_RED << "this software must be run as root!" << term::RESET << std::endl;
		return EXIT_FAILURE;
	}

	std::string file_name;
	for (size_t i = 1; i < argc; i++) {
		file_name += argv[i];
	}
	
	if (file_name.empty()) {
		std::cerr << term::RED << "Error: " << term::LIGHT_RED << "please specify an image file to read." << std::endl;
		return EXIT_FAILURE;
	}

	fs::FilePath file{file_name};
	if (!file.exists()) {
		std::cerr << term::RED << "Error: " << term::LIGHT_RED << "the file '" << file.to_string() << "' cannot be found." << std::endl;
		return EXIT_FAILURE;
	}

	// Create the context.
	Context context{};
	context.load_config_file();

	// Setup the GPIO and display R, G and B colors to the RGB led to confirm it works.
	setup_gpio(context);
	hello_world_led(context);

	// Read the image file with STBimage.
	std::cout << "Reading file... ";
	int width, height, channels;
	auto image = stbi_load(file.to_string().c_str(), &width, &height, &channels, 4);
	std::cout << "(Got width: " << std::to_string(width) << ", height: " << std::to_string(height) << " and channels: " << std::to_string(channels) << ")\n";

	// Reads every pixels.
	for (size_t y = 0; y < height; y++)
		for (size_t x = 0; x < width; x++) {
			auto color = get_pixel(image, width, 4, x, y);
			std::cout << term::erase_current_line << color.to_string(false) << " at x" << std::to_string(x) << " y" << std::to_string(y) << "\r" << std::flush;
			// Display the color to the led if the opacity isn't null.
			if (color.alpha_as_int() != 0) {
				// Write PWM signals to the led wit the RGB squared values. (Please see https://www.youtube.com/watch?v=LKnqECcg6Gw)
				softPwmWrite(context.get_red_pin(), static_cast<int>(pow(color.red(), 2.f) * 255.f));
				softPwmWrite(context.get_green_pin(), static_cast<int>(pow(color.green(), 2.f) * 255.f));
				softPwmWrite(context.get_blue_pin(), static_cast<int>(pow(color.blue(), 2.f) * 255.f));
			}
			// @TODO: Replace with sleep when lambdacommon will be fixed.
			delay(250);
			set_led_off(context);
		}

	std::cout << std::endl;

	// Free the image.
	stbi_image_free(image);
	return EXIT_SUCCESS;
}

void setup_gpio(const Context &context)
{
	wiringPiSetup();
	pinMode(context.get_red_pin(), OUTPUT);
	softPwmCreate(context.get_red_pin(), LOW, 255);
	pinMode(context.get_green_pin(), OUTPUT);
	softPwmCreate(context.get_green_pin(), LOW, 255);
	pinMode(context.get_blue_pin(), OUTPUT);
	softPwmCreate(context.get_blue_pin(), LOW, 255);
}

void set_led_off(const Context &context)
{
	softPwmWrite(context.get_red_pin(), LOW);
	softPwmWrite(context.get_green_pin(), LOW);
	softPwmWrite(context.get_blue_pin(), LOW);
}

void hello_world_led(const Context &context)
{
	set_led_off(context);
	softPwmWrite(context.get_red_pin(), 255);
	sleep(1000);
	set_led_off(context);
	softPwmWrite(context.get_green_pin(), 255);
	sleep(1000);
	set_led_off(context);
	softPwmWrite(context.get_blue_pin(), 255);
	sleep(1000);
	set_led_off(context);
}

lambdacommon::Color get_pixel(stbi_uc *image, int width, int channels, size_t x, size_t y)
{
	uint8_t alpha = 0xFF;
	if (channels == 4)
		alpha = image[4 * (y * width + x) + 3];
	return lambdacommon::color::from_int_rgba(image[channels * (y * width + x)], image[channels * (y * width + x) + 1], image[channels * (y * width + x) + 2], alpha);
}
