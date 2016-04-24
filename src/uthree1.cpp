#include "glpp/uthree.hpp"

using glpp::three;

int main(int argc, char const *argv[])
{
	float angle = 0.0;
	int width = 640;
	int height = 480;
	auto window = glpp::init(width,height);

	auto scene = new Scene();
	auto camera = new PerspectiveCamera( 70, window.innerRatio, 1, 1000 );
	camera.position.z = 400;
	auto texture = TextureLoader().load( 'crate.gif' );
	auto geometry = AssimpLoader().loadgeo( "bunny.ply";)
	//auto geometry = new BoxBufferGeometry( 200, 200, 200 );
	auto material = new MeshBasicMaterial();
	material->texture = texture;
	auto mesh = new Mesh( geometry, material );
	scene.add( mesh );

	auto renderer = new Renderer();
	//auto renderer.setPixelRatio( window.devicePixelRatio );
	auto renderer.setSize( window.innerWidth, window.innerHeight );
	do {
		renderer->render(scene);
		glfwSwapBuffers(window);
		glfwPollEvents();
	} 
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
	glfwTerminate();

	return 0;
}