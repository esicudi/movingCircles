#pragma once
#ifndef __CIRCLE_H__
#define __CIRCLE_H__

struct circle_t
{
	vec2	center=vec2(0);		// 2D position for translation
	float	radius=1.0f;		// radius
	float	theta=0.0f;			// rotation angle
	vec4	color;				// RGBA color in [0,1]
	mat4	model_matrix;		// modeling transformation
	vec2	velocity;			// velocity of circle

	// public functions
	void	update( float t );
};

inline float randf(float m, float M)
{
	float r = rand() / float(RAND_MAX - 1);
	return m + (M - m) * r;
}

inline std::vector<circle_t> create_circles(int N)
{
	std::vector<circle_t> circles;
	circle_t c;
	circle_t c1;
	int size;
	
	for (int k = 0; k < N; k++)
	{
		vec2 pos;
		pos.x = randf(-1.0f, 1.0f);
		pos.y = randf(-1.0f, 1.0f);
		float radius = randf(0.2f, 1.0f) / sqrtf(float(N));

		// initial collision
		bool col = false;
		size = circles.size();
		for (int i = 0; i < size; i++) {
			c1 = circles[i];
			vec2 center = c1.center;
			float r = c1.radius;
			float d;
			d = sqrtf(powf(pos.x - center.x, 2) + powf(pos.y - center.y, 2));
			if (d <= (r + radius + 0.01)) { col = true; break; }
		}

		if (pos.x - (radius * 9.0f / 16.0f) < -1.0f) { col = true; }
		else if ((pos.y - radius) < -1.0f) { col = true; }
		else if (pos.x + (radius * 9.0f / 16.0f) > 1.0f) { col = true; }
		else if ((pos.y + radius) > 1.0f) { col = true; }

		if (col)
		{
			k--;
			continue;
		}

		vec4 color;
		color.r = randf(0.1f, 1.0f);
		color.g = randf(0.1f, 1.0f);
		color.b = randf(0.1f, 1.0f);
		color.a = 1.0f;

		c = { pos,radius,0.0f,color };

		c.velocity.x = randf(-0.8f, 0.8f);
		c.velocity.y = randf(-0.8f, 0.8f);

		circles.emplace_back(c);
	}

	return circles;
}

inline void circle_t::update( float t )
{
	

	// these transformations will be explained in later transformation lecture
	mat4 scale_matrix =
	{
		radius, 0, 0, 0,
		0, radius, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	mat4 rotation_matrix =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	center = velocity * t + center;
	mat4 translate_matrix =
	{
		1, 0, 0, center.x,
		0, 1, 0, center.y,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	
	model_matrix = translate_matrix*rotation_matrix*scale_matrix;
}

#endif
