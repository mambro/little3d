#pragma once
#include "glpp/draw.hpp"
#include <Eigen/Geometry>

namespace glpp
{
	namespace three
	{
		class Object3D;
		class Mesh;
		class Geometry;
		class Material;
		class Renderer;

		using BBox = Eigen::AlignedBox<float>;
		using Color4 = Eigen::Vector4f;

		class Plane: public Eigen::Vector4f
		{
		public:
			void set(const Eigen::Vector3f & normal,const Eigen::Vector3f & point);

			// float distance(const Eigen::Vector3f & pt);

			// Plane transform(const Eigen::Transform & pos);
		};

		class Sphere: public Eigen::Vector4f
		{
		public:
			void set(const Eigen::Vector3f & c, float r);
		};

		class Frustum
		{
		public:
			/// given abirtrary projection matrix build the clipping planes in WORLD coordinates
			void fromMatrix(const Eigen::Matrix4f & mtx);

			int intersect(const BBox & bbox) const;
			int intersect(const Sphere & sphere) const;
			bool contains(const Eigen::Vector3f & pt) const;
			// clone
			// set
			// copy
			std::array<Plane,6> planes;
		};

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
			Eigen::Transform pose; // in parent
			BBox bboxLocal; // TBD
			BBox bboxWorld; // TBD
			Eigen::Matrix4 local2parent;
			Eigen::Matrix4 local2world;
			bool visible = true;
			bool invalidWorld;

			void invalidateWorld();

			void updateWorld();

			void updateWorld(std::shared_ptr<Object3D> p);

			void add(std::shared_ptr<Object3D> x);

			virtual Object3D * clone() const { return nullptr; }
		};		

		class Light: public Object3D
		{
		};

		class DirectionalLight: public Light
		{
		};

		class SpotLight: public Light
		{
		};

		class Material
		{

		};		

		class MeshBasicMaterial: public Material
		{
		public:
			bool wireframe;
			Color4 color;
			// shading
			// vertexColors
			// fog
			// ...
		};

		class MeshPhongMaterial: public Material
		{

		};

		class Geometry
		{
		public:
			std::string name;
			int id;
			Eigen::AlignedBox<float> bbox;
			Eigen::Matrix<float,Eigen::Dynamic,3> vertices; // +normals + texture coords
			Eigen::Matrix<int,Eigen::Dynamic,3> triangles;

			void updateGL();

			void updateBBox();

			// colors
			// faces
			// bsphere
		protected:
			// VBO / TBO
		};		

		class Camera: public Object3D
		{
		public:
			Eigen::Matrix4f matrixWorldinverse;
			Eigen::Matrix4f projectionMatrix;

			void lookAt(Eigen::Vector3f eye, Eigen::Vector3f center, Eigen::Vector3f up);
			virtual void updateProjectionMatrix() = 0;


			// getWorldDirection
			// clone
			// copy 
		};		

		class PerspectiveCamera: public Camera
		{
		public:
			float fovy;
			float ration;
			Eigen::Vector2f nearfar;
			// NOTE: camera.setViewOffset + MISSING oblique
			void updateProjectionMatrix();

//			PerspectiveCamera * clone() const override { return new PerspectiveCamera(); }
		};		

		class OrthograpicCamera: public Camera
		{
		public:
			float ration;
			Eigen::Vector2f nearfar;
			// NOTE: camera.setViewOffset + MISSING oblique
			void updateProjectionMatrix();

//			OrthograpicCamera * clone() const override { return new OrthograpicCamera(); }
		};		

		class Mesh: public Object3D
		{
		public:
			Mesh(std::shared_ptr<Geometry>,std::shared_ptr<Material>);

			// raycast
			// clone

			std::shared_ptr<Geometry> geometry;
			std::shared_ptr<Material> material;
		};	

		class Texture
		{

		};

		// FBO
		class RenderTarget
		{

		};

		class Renderer
		{
		public:
			// control Viewport
			// control Stencil
			// control ClearColor
			// manual clear
			void render (std::shared_ptr<Object3D> scene, std::shared_ptr<Camera> camera); //, renderTarget, forceClear );
		protected:
			void collect(Object3D * o);
			std::map<Material*,std::list<Mesh*>> renderable;
			std::list<Light*> lights;
		};
	}
}
