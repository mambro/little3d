#pragma once

namespace glpp
{
	namespace three
	{
		class Object3D;
		class Mesh;
		class Geometry;
		class Material;
		class Renderer;

		class Object3D
		{
		public:
			//uuid
			//add
			//remove
			std::string name;
			int id;
			std::shared_ptr<Object3D> parent;
			std::vector<std::shared_ptr<Object3D> > children;
			Eigen::Transform pose;
			Eigen::Matrix4 local2parent;
			Eigen::Matrix4 local2world;
			bool visible = true;
		};		

		class Material
		{

		};		

		class MeshBasicMaterial: public Material
		{
		public:
			// color
			// wireframe
			// shading
			// vertexColors
			// fog
			// ...
		};

		class Geometry
		{
		public:
			std::string name;
			int id;
			// vertices
			// colors
			// faces
			// bbox bsphere
		};		

		class Camera: public Object3D
		{
		public:
			Eigen::Matrix4f matrixWorldinverse;
			Eigen::Matrix4f projectionMatrix;

			// lookAt
			// getWorldDirection
			// clone
			// copy 
		};		

		class PerspectiveCamera: public Camera
		{
		public:
			// NOTE: camera.setViewOffset + MISSING oblique
			// updateProjectionMatrix 
		};		


		class Mesh: public Object3D
		{
		public:
			Mesh(std::shared_ptr<Geometry>,std::shared_ptr<Material>);

			// raycast
			// clone
		};		

		class Renderer
		{
		public:
			// control Viewport
			// control Stencil
			// control ClearColor
			// manual clear
			void render ( scene, camera, renderTarget, forceClear );
		};
	}
}
