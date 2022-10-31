// Tyler Morrill
// September 24, 2022
// 
// 
// Here is code inspired from Peter Shirley to create a simple ray tracer. I will be modifying this file as I create my 
// rendering framework.

#include "rt_weekend.h"

#include "stb-master/stb_image_write.h"
#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "moving_sphere.h"
#include "bvh.h"
#include "aarect.h"
#include "box.h"

#include <iostream>

hittable_list cornell_box() {
	hittable_list objects;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));
	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
	objects.add(make_shared<xz_rect>(213, 343, 227, 332, 554, light));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
	objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

	shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
	box1 = make_shared<rotate_y>(box1, 15);
	box1 = make_shared<translate>(box1, vec3(265, 0, 295));
	objects.add(box1);

	shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
	box2 = make_shared<rotate_y>(box2, -18);
	box2 = make_shared<translate>(box2, vec3(130, 0, 65));
	objects.add(box2);

	return objects;
}

hittable_list earth() {
	auto earth_texture = make_shared<image_texture>("earthmap.jpg");
	auto earth_surface = make_shared<lambertian>(earth_texture);
	auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);
	return hittable_list(globe);
}

hittable_list random_scene() {
	hittable_list world;

	auto checkered = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
	auto ground_material = make_shared<lambertian>(checkered);
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_double();
			point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					auto center2 = center + vec3(0, random_double(0,.5), 0);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
				} else if (choose_mat < 0.95) {
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_double(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				} else {
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));
	
	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

	return world;
}

hittable_list two_perlin_spheres() {
	hittable_list objects;

	auto pertext = make_shared<noise_texture>(6);
	objects.add(make_shared<sphere>(point3(0, -1000, 0), 1000, make_shared<lambertian>(pertext)));
	objects.add(make_shared<sphere>(point3(0, 2, 0), 2, make_shared<lambertian>(pertext)));
	return objects;
}

// Determines the color of the ray based on the normal vector to the sphere.
color ray_color(const ray& r, const color& background, const hittable& world, int depth) {
	hit_record rec;

	// If we exceed the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0,0,0);

	// If the ray hits nothing, return the background color.
	if (!world.hit(r, 0.001, infinity, rec))
		return background;

	ray scattered;
	color attenuation;
	color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

	if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
		return emitted;
	
	return emitted + attenuation * ray_color(scattered, background, world, depth-1);
}

int main() {

	// Image
	auto aspect_ratio = 3.0 / 2.0;
	int image_width = 400;
	int image_height = static_cast<int>(image_width / aspect_ratio);
	int samples_per_pixel = 300;
	int max_depth = 50;

	// World
	hittable_list world;
	hittable_list temp;
	point3 lookfrom;
	point3 lookat;
	auto fov = 40.0;
	auto aperture = 0.0;
	color background(0,0,0);

	int scene = 3;
	switch (scene)
	{
	case 0: // Random scene.
		world.add(make_shared<bvh_node>(random_scene(), 0, 0));
		background = color(0.7, 0.8, 1.0);
		lookfrom = point3(13, 2, 3);
		lookat = point3(0, 0, 0);
		fov = 20.0;
		aperture = 0.1;
		break;
	
	case 1: // Three spheres.
		world.add(make_shared<sphere>(point3(0, -1000.5, -1.0), 1000, make_shared<lambertian>(color(0.8, 0.8, 0.0))));
		world.add(make_shared<sphere>(point3(0, 0, -1.0), 0.5, make_shared<dielectric>(1.8)));
		world.add(make_shared<sphere>(point3(-0.6, 0, -3.0), 0.5, make_shared<lambertian>(color(0.8, 0.8, 0.8))));
		world.add(make_shared<sphere>(point3(0.6, 0, -3.0), 0.5, make_shared<lambertian>(color(0.8, 0.6, 0.2))));

	case 2: // Earth with square light above it.
		temp = earth();
		temp.add(make_shared<xz_rect>(-2, 2, -2, 2, 5, make_shared<diffuse_light>(color(5, 5, 5))));
		//world.add(make_shared<sphere>(point3(0, -22, 0), 20, make_shared<metal>(color(1,1,1), 0)));
		temp.add(make_shared<sphere>(point3(0, -1002, 0), 1000, make_shared<lambertian>(color(0.7,0.7,0.7))));
		world.add(make_shared<bvh_node>(temp, 0, 0));
		background = color(0, 0, 0);
		lookfrom = point3(20, 10, 0);
		lookat = point3(0, 0, 0);
		fov = 40.0;
		break;
	
	case 3: // Cornell box with two boxes.
		temp = cornell_box();
		aspect_ratio = 1.0;
		image_width = 600;
		image_height = static_cast<int>(image_width / aspect_ratio);
		samples_per_pixel = 200;
		background = color(0,0,0);
		lookfrom = point3(278, 278, -800);
		lookat = point3(278, 278, 0);
		fov = 40.0;
		world.add(make_shared<bvh_node>(temp, 0, 0));
		break;

	case 4:
		temp = two_perlin_spheres();
		background = color(0.7, 0.8, 1.0);
		lookfrom = point3(13, 2, 3);
		lookat = point3(0, 0, 0);
		fov = 20.0;
		world.add(make_shared<bvh_node>(temp,0,0));
		break;
	}
	 
	// Camera
	vec3 vup(0, 1, 0);
	auto dist_to_focus = 10; //(lookfrom-lookat).length();
	camera cam(lookfrom, lookat, vup, fov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);

	// Render
	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
	for (int j = image_height-1;j >= 0; j--) {
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; i++) {
			color pixel_color(0, 0, 0);
			// Performs the random sampling used in anti-aliasing. Might want to condense writing color to the image in the future. I suspect it will be when we create materials because we will have the color predetermined.
			for (int s = 0; s < samples_per_pixel; s++) {
				auto u = (i + random_double()) / (image_width-1);
				auto v = (j + random_double()) / (image_height-1);
				ray r = cam.get_ray(u, v);
				pixel_color += ray_color(r, background, world, max_depth);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
	}

	std::cerr << "\nDone.\n";

}