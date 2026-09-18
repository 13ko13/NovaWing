#include "SphereShape.h"

SphereShape::SphereShape(float radius):
	ColliderShape(Shape::Sphere),
	m_radius(radius)
{
}
