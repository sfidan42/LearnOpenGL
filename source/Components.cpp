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

	auto& instances = registry.get<ModelComponent>(inst.modelEntity).instances;

	erase(instances, instanceEnt);
}

void onInstanceUpdated(entt::registry& registry, entt::entity instanceEnt)
{
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	// Previous value is available through the patch mechanism
	registry.patch<InstanceComponent>(instanceEnt, [&](auto& previous)
	{
		if(previous.modelEntity == inst.modelEntity)
			return;

		// Remove from old model
		if(registry.valid(previous.modelEntity))
		{
			auto& oldList =
				registry.get<ModelComponent>(previous.modelEntity).instances;
			erase(oldList, instanceEnt);
		}

		// Add to new model
		if(registry.valid(inst.modelEntity))
		{
			auto& newList =
				registry.get<ModelComponent>(inst.modelEntity).instances;
			newList.push_back(instanceEnt);
		}
	});
}

void setupInstanceTracking(entt::registry& registry)
{
	registry.on_construct<InstanceComponent>()
			.connect<&onInstanceAdded>();

	registry.on_destroy<InstanceComponent>()
			.connect<&onInstanceRemoved>();

	registry.on_update<InstanceComponent>()
			.connect<&onInstanceUpdated>();
}
