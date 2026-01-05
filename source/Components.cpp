#include "Components.hpp"
#include <glm/ext.hpp>

void onInstanceAdded(entt::registry& registry, const entt::entity instanceEnt)
{
	auto& [modelEntity] = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(modelEntity))
		return;

	auto& modelComp = registry.get<ModelComponent>(modelEntity);

	modelComp.instances.push_back(instanceEnt);
}

void onInstanceRemoved(entt::registry& registry, const entt::entity instanceEnt)
{
	// The component still exists at this point
	auto& [modelEntity] = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(modelEntity))
		return;

	// Check if the ModelComponent still exists before accessing it
	if(!registry.all_of<ModelComponent>(modelEntity))
		return;

	auto& instances = registry.get<ModelComponent>(modelEntity).instances;

	erase(instances, instanceEnt);
}

mat4 TransformComponent::bake() const
{
	mat4 modelMat(1.0f);
	modelMat = translate(modelMat, position);
	modelMat = rotate(modelMat, rotation.x, {1, 0, 0});
	modelMat = rotate(modelMat, rotation.y, {0, 1, 0});
	modelMat = rotate(modelMat, rotation.z, {0, 0, 1});
	modelMat = glm::scale(modelMat, scale);
	return modelMat;
}

void ModelComponent::drawInstanced(const Shader& shader) const
{
	if (instances.empty())
		return;
	shader.use();
	model.drawInstanced(shader, instanceMatrices);
}

void setupInstanceTracking(entt::registry& registry)
{
	registry.on_construct<InstanceComponent>()
			.connect<&onInstanceAdded>();

	registry.on_destroy<InstanceComponent>()
			.connect<&onInstanceRemoved>();
}
