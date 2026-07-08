#pragma once

enum ShaderType : unsigned int {
	VERTEX = 0b00000001,
	PIXEL = 0b00000010,
	GEOMETRY = 0b00000100,

	ALL = VERTEX | PIXEL | GEOMETRY
};