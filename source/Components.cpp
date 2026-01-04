#include "Components.hpp"

void onInstanceAdded(entt::registry& registry, entt::entity instanceEnt)
{
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(inst.modelEntity))
		return;

	auto& modelComp = registry.get<ModelComponent>(inst.modelEntity);

	modelComp.instances.push_back(instanceEnt);
}

void onInstanceRemoved(entt::registry& registry, entt::entity instanceEnt)
{
	// The component still exists at this point
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(inst.modelEntity))
		return;

	// Check if the ModelComponent still exists before accessing it
	if(!registry.all_of<ModelComponent>(inst.modelEntity))
		return;

	auto& instances = registry.get<ModelComponent>(inst.modelEntity).instances;

	erase(instances, instanceEnt);
}

void setupInstanceTracking(entt::registry& registry)
{
	registry.on_construct<InstanceComponent>()
			.connect<&onInstanceAdded>();

	registry.on_destroy<InstanceComponent>()
			.connect<&onInstanceRemoved>();
}
