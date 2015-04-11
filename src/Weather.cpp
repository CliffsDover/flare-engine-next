/*
Copyright © 2015 Wolfgang Pirker

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include <cmath>
#include "CommonIncludes.h"
#include "SDLHardwareRenderDevice.h"
#include "SDLSoftwareRenderDevice.h"

#include "Weather.h"
#include "WeatherClimate.h"
#include "Utils.h"
#include "SharedGameResources.h"


//==============================================================================
WeatherCloud::WeatherCloud(WeatherCloud::SizeType c_size, int s)
	: spr_cloud(NULL)
{
	Image *img_cloud = render_device->loadImage("images/weather/clouds.png",
				"Couldn't load cloud image!", false);
	if (!img_cloud) return;
	img_cloud->ref();
	//if (c_size == SizeType(0)){
		spr_cloud = img_cloud->createSprite();
		spr_cloud->setClipW(600);
		spr_cloud->setClipX(s*600);
	//}
	/*if (c_size == SizeType(1)){
	// FIXME: SDL hardware renderer scaled images with graphic glitches
		Image *resized = img_cloud->resize(img_cloud->getWidth()*1.5, img_cloud->getHeight()*1.5);
		spr_cloud = resized->createSprite();
		spr_cloud->setClipW(900);
		spr_cloud->setClipX(s*900);
	}
	else {//(cloud->getSize() == 2){
	// FIXME: SDL hardware renderer scaled images with graphic glitches
		Image *resized = img_cloud->resize(img_cloud->getWidth()*2, img_cloud->getHeight()*2);
		spr_cloud = resized->createSprite();
		spr_cloud->setClipW(1200);
		spr_cloud->setClipX(s*1200);
	}*/
	img_cloud->unref();

};


WeatherCloud::~WeatherCloud(){
};

Sprite* WeatherCloud::getSprite(){
	if (!WeatherCloud::spr_cloud) return NULL;
	return WeatherCloud::spr_cloud;
}

/*void WeatherCloud::setToBeRendered(bool flag){
	to_be_rendered = flag;
}*/

ListWeatherCloud::ListWeatherCloud()
	: is_snow(false)
	, direction(2)
	, speed(1.0)
	, cycle_i(0)
	, cycle_max(0)
	, time_of_rain(0)
	, cloud_distance(0)
	, is_strong_rainfall(true)
	, flakes_arr_initialized(false)
	, clouds_arr_initialized(false)
	, img_rainfall(NULL)
	, spr_flake(NULL)
	, cloud_state({})
	, flake_state({})
{
};

ListWeatherCloud::~ListWeatherCloud(){
	if (!cloud_list.empty()){
		cloud_list.clear();
	}
	if (img_rainfall != NULL) render_device->freeImage(img_rainfall);
	if (img_cloud != NULL){
		render_device->freeImage(img_cloud);
		img_cloud->unref();
	}
};

ListWeatherCloud ListWeatherCloud::instance;

void ListWeatherCloud::setSnow(bool is_snow_a){
    is_snow = is_snow_a;
}
/* TODO: fog feature
void ListWeatherCloud::setFog(bool is_fog_a){
    is_fog = is_fog_a;
}
*/


void ListWeatherCloud::setWindDirection(FPoint p1, FPoint p2){
    ListWeatherCloud::direction = calcDirection(p1, p2);
}

int ListWeatherCloud::getWindDirection(){
    return ListWeatherCloud::direction;
}

void ListWeatherCloud::setWindForce(float speed_a){ // speed in approximate tiles per second
    speed=speed_a;
}

float ListWeatherCloud::getWindForce(){
    return speed;
}

void ListWeatherCloud::logicClouds() {
    cycle_i+=1;
}

bool ListWeatherCloud::logicClouds(int base_cloudiness_a, long cycle_max_a){
    int cloudiness;
    cycle_max = cycle_max_a + randBetween(-5000,5000); // TODO: needs adjustment
    cloudiness=base_cloudiness_a + randBetween(-40, 40);

    cloud_distance = 800 / cloudiness;

    if (cloud_list.empty()){

        const int half_max_cycle = cycle_max/2;
        createClouds(cloudiness);
        // rain variable: time_of_rain
          // determines after which time of the cycle it should
          // start to rain; no rain if time_of_rain>cycle_max;
          // depends both on cloudiness and some randomness
        time_of_rain = cycle_max * ( 10/cloudiness) + randBetween(0, half_max_cycle);
        // determine if it should be strong rainfall, let it depend on cloudiness and time_of_rain
        if ((cloudiness*time_of_rain/cycle_max>=30)) is_strong_rainfall = true;

        // instantiate RenderDevice Images
        if (is_snow){
            img_rainfall = render_device->loadImage("images/weather/snow_transparent.png",
                "Couldn't load snow image!", false);
        }
        else{
            img_rainfall = render_device->loadImage("images/weather/rain.png",
                "Couldn't load rain image!", false);
        }
        if (img_rainfall){
            spr_flake = img_rainfall->createSprite();
            img_rainfall->unref();
        }


        cycle_i = 0;
    }
    return true;
}

void ListWeatherCloud::renderClouds(){
	Rect screen_size = render_device->getContextSize();

	//if (cycle_i > time_of_rain) renderRainfall();
    renderRainfall(); // TODO: remove this line, uncomment previous

	if ( (clouds_arr_initialized) && (!cloud_list.empty()) ){
		std::list<WeatherCloud>::iterator it=cloud_list.begin();
		WeatherCloud *cloud;
		Sprite *spr_cloud;
		Point p;
		int i, j, nr=0;
		Point dest_p;

		Rect screen_size = render_device->getContextSize();
		Rect spr_size;

		i=-RADIUS;
		j=0; // -> with j=-RADIUS: segmentation faults quite common

		while (j < RADIUS){
			// use a cloud from the list several times if needed
			if ((it == cloud_list.end())) it=cloud_list.begin();

			cloud = &*it;
			if (cloud==NULL){
				it++;
				continue; // break;
			}

			spr_cloud = cloud->getSprite();

			if (spr_cloud==NULL) break;

			// distance of cloud_state[nr][4] - mapr->cam.x is getting huge, then:
			if (abs(floor(mapr->cam.x) - cloud_state[nr][4]) > 2*RADIUS){
				cloud_state[nr][4] = floor(mapr->cam.x);
			}
			if (abs(floor(mapr->cam.y) - cloud_state[nr][5]) > 2*RADIUS){
				cloud_state[nr][5] = floor(mapr->cam.y);
			}

			p = map_to_screen(cloud_state[nr][4] + i, cloud_state[nr][5] + j, mapr->cam.x, mapr->cam.y);
			if (cycle_i % RENDER_CHANGE_AFTER == 0) {
				// TODO: should depend on wind direction and perhaps speed;
				  // RENDER_CHANGE_AFTER could be a class variable which depends on the variable speed
				cloud_state[nr][2] = cloud_state[nr][2] + randBetween(1,2);
				cloud_state[nr][3] = cloud_state[nr][3] + randBetween(1,2);
				direction = calcDirection(0.0,0.0, cloud_state[nr][2], cloud_state[nr][3]);
			} // FIXME: overflow possible (if playing many hours)

			dest_p.x = p.x + (cloud_state[nr][2] % screen_size.w) - screen_size.w/2;
			dest_p.y = p.y + (cloud_state[nr][3] % screen_size.h/2) - screen_size.h/4;

			spr_cloud->setOffset(cloud_state[nr][1],cloud_state[nr][1]);
			spr_cloud->setDest(dest_p);

			render_device->render(spr_cloud);

			i+=cloud_distance;
			if (i>RADIUS){
				i=-RADIUS;
				j+=cloud_distance;
			}
			it++;
			nr++;
        }

	}
}

ListWeatherCloud* ListWeatherCloud::getInstance() { return &ListWeatherCloud::instance; }

void ListWeatherCloud::createClouds(int cloudiness){
    WeatherCloud::SizeType size;
    int shape=0;
    std::list<WeatherCloud>::iterator it=cloud_list.begin();
    int i=0;
    Point p;

    Rect screen_size = render_device->getContextSize();

    while (cloudiness>4){ // create clouds
		if (i >= MAX_NUMBER_OF_CLOUDS-1) break;

		size=WeatherCloud::SizeType(randBetween(0,2));

		cloud_state[i][0] = size;
		cloud_state[i][1] = randBetween(-4,4);
		//std::cout<<"cloud distance: " << cloud_distance << std::endl;
		if (cloud_distance < 14) cloud_distance = 14; // => looks bad if too narrow
		cloud_state[i][2] = randBetween(-cloud_distance*6,cloud_distance*6);
		cloud_state[i][3] = randBetween(-cloud_distance*6,cloud_distance*6);
		cloud_state[i][4] = floor(mapr->cam.x);
		cloud_state[i][5] = floor(mapr->cam.y);

		// fill cloud_list
		shape=randBetween(0,5);
		cloud_list.insert(it, WeatherCloud(size, shape));
		cloudiness-=(size+2)*3;
		i+=1;
		it++;
    }
    clouds_arr_initialized = true;
}

void ListWeatherCloud::logicDebug(){
    if (cycle_i==1){
        std::cout<<"time of rain: " << time_of_rain << std::endl;
    }
    if (cycle_i==time_of_rain){
        std::cout<<"time rainfall starts" << std::endl;
    }
    if (cycle_i==cycle_max) {
        std::cout<<"time cycle ended" << std::endl;
        std::cout<<"clouds should fade off now..." << std::endl;
    }

}

void ListWeatherCloud::renderRainfall(){
    if (ListWeatherCloud::is_snow){
        renderSnow();
    }
    else renderRain();
}

void ListWeatherCloud::renderSnow(){
	if (!img_rainfall) return;
    int nr = 0;
    int i = 0;
    int j = 0;
    int r3 = 1; // changes type
    int r4 = 2;// changes offset
    int density = 3;

    Rect screen_size = render_device->getContextSize();
    Point p;
    FPoint fp; // needed to check if is_valid_position
    Rect spr_size;

    if (is_strong_rainfall){
        density = 2;
    }

    // initialize the flake_state Array, if it isn't yet
    if (!flakes_arr_initialized){

        flakes_arr_initialized = true;
        while (nr < 20){
            r3 = randBetween(0,6);
            r4 = randBetween(-4,4);
            flake_state[nr][0] = r3; // type
            flake_state[nr][1] = r4;
            flake_state[nr][2] = 0;
            flake_state[nr][3] = 0;
            nr+=1;
        }

    }

    i = -RADIUS;
    j = -RADIUS;

    while(j < RADIUS){
        if (nr>19) nr=0;
        // update of position info should be rather slow...
          // otherwise the snowflakes appear to move with the hero
        if ((cycle_i % (RENDER_CHANGE_AFTER * 10) == 0) || mapr->map_change) {
            p_flakes = floor(mapr->cam);
        }
        spr_flake->setClipW(8);
        spr_flake->setClipH(10);
        spr_flake->setClipX(flake_state[nr][0]*9);

        // TODO: take into account wind direction (variable 'direction')
        p = map_to_screen(p_flakes.x + i, p_flakes.y + j, mapr->cam.x, mapr->cam.y);
        if (cycle_i % RENDER_CHANGE_AFTER == 0) {
            flake_state[nr][2] = flake_state[nr][2] + randBetween(-1,1);
            flake_state[nr][3] = flake_state[nr][3] + randBetween(-1,2);
        }

        spr_flake->setOffset(flake_state[nr][1],0);
        spr_flake->setDestX(p.x + flake_state[nr][2]);
        spr_flake->setDestY(p.y + (flake_state[nr][3] % (screen_size.h/4)) - screen_size.h/4);

        if (mapr->collider.is_valid_position(p_flakes.x + i, p_flakes.y + j, MOVEMENT_FLYING, false)){
			// fade off effect for snowflakes near the ground
			if (flake_state[nr][3] % (screen_size.h/4) > screen_size.h/6){
				spr_flake->setClipY(10);
			}
			else spr_flake->setClipY(0);

			render_device->render(spr_flake);

        }
        i+=density;
        if (i>RADIUS){
            i= -RADIUS;
            j+=density;
        }
        nr+=1;
    }
}

void ListWeatherCloud::renderRain(){


}

WeatherManager::WeatherManager()
	: initialized(false)
	, list_weather_cloud(NULL)
{
    WeatherClimate *klima = WeatherClimate::getInstance();
    enabled = klima->getEnabled();
};

WeatherManager::~WeatherManager(){
};

//------------------------------------------------------------------------------
void WeatherManager::init(){
    //ListWeatherCloud *list_weather_cloud;
    WeatherClimate *klima = WeatherClimate::getInstance();
    SeasonType season_type = SeasonType(klima->getSeason());
    HumidityType humidity_type = HumidityType(klima->getHumidity());

    int cloudiness = 0;
    //int fogginess = 20;
    //int wind_direction = 2;
    //float wind_speed = 1.0;
    bool is_snow = false;
    long cycle_max = 40000; // TODO: should be influenced by WeatherClimate settings

    if (season_type==klima->WINTER) is_snow=true;
    // Note: Likeliness of rain and clouds is not ONLY influenced by the
        // WeatherClimate setting, but is partly also random
        // the random part of both cloudiness and cycle_max is done in
        // ListWeatherClouds
    cloudiness = 10;
    // TODO: wind variables changes
      // ...
    if (humidity_type==WeatherClimate::NORMAL){
        cloudiness+=16;
    }
    else if(humidity_type==WeatherClimate::WET){
        cloudiness+=32;
    } // else DRY, stays
    if (season_type==WeatherClimate::SUMMER){ // dryer in the summer
        cloudiness-=10;
    }
    else if (season_type==WeatherClimate::AUTUNM){
        cloudiness+=10;
        //fogginess = 50; // TODO, fog
    }
    // ListWeatherCloud mustn't use WeatherClimate!
    list_weather_cloud = ListWeatherCloud::getInstance();
    list_weather_cloud->setSnow(is_snow);
    list_weather_cloud->logicClouds(cloudiness, cycle_max);
}

void WeatherManager::logic(){
    if (!initialized) {
        WeatherManager::init();
        initialized=true;
        return;
    }
    list_weather_cloud->logicClouds();

}

void WeatherManager::render(){
    //ListWeatherCloud *list_weather_cloud; // TODO: use list_weather_cloud as class variable!
    //list_weather_cloud = ListWeatherCloud::getInstance();

    if (!initialized) return;
    list_weather_cloud->renderClouds();
}

bool WeatherManager::getEnabledFlag(){
    return enabled;
}


//WeatherManager* WeatherManager::getInstance() { return &WeatherManager::instance; }
