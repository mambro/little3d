// TODO: decide if OpenGL stays local in DOM or not

#include "little3d/uthree.hpp"

namespace little3d {
	namespace three {

		/// down invalidate (TODO: optimize)
		void Object3D::invalidateWorld()
		{
			invalidWorld = true;
		}

		// entrypoint
		void Object3D::updateWorld()
		{
			if(invalidWorld)
				updateWorld(parent);
		}

		/// down recursion validation (ALL children)
		void Object3D::updateWorld(std::shared_ptr<Object3D> p)
		{
			invalidWorld = false;
			local2world = p ? p->local2world * local2parent : local2parent;
			for(auto & c: children)
				c->updateWorld(this); // TODO this -> shared
		}


		void Object3D::add(std::shared_ptr<Object3D> x)
		{
			x->parent = this; // TODO this->shared
			childen().append(x);
			x->invalidateWorld();
		}

		void Renderer::collect(Object3D * o, const Frustum & f)
		{
			if(!o->visible)
				return;

			// TODO check culled
			Mesh * m = dynamic_cast<Mesh*>(o);
			if(m)
			{
				renderable[m->material].push_back(m->geometry);
			}
			else
			{
				Light * l = dynamic_cast<Light*>(o);
				if(l)
					lights.push_back(l);
			}
			for(auto p: o->children)
				collect(p.get(),f);
		}

		// TODO sorting for transparency
		// TODO more passes
		void Renderer::render (std::shared_ptr<Object3D> scene, std::shared_ptr<Camera> camera,, std::shared_ptr<RenderTarget>)
		{
			scene->updateWorld();
			camera->updateWorld();
			camera->matrixWorldinverse = camera->local2world.inverse();

			Frustum f;
			f.fromMatrix(camera.projectionMatrix * camera.local2world);

			// TODO updateBBox()
			// extract renderable AND lights
			renderable.clear();
			lights.clear();
			collect(scene.get(),f);

			for(auto & p: renderable)
			{
				// activate material p.first
				p.first->activate();
				// render gemetry p.second
			}
		}
	}
}