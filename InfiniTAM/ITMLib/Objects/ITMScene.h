// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#pragma once

#include "../Utils/ITMLibDefines.h"

#include "ITMSceneParams.h"
#include "ITMLocalVBA.h"
#include "ITMGlobalCache.h"

namespace ITMLib
{
	namespace Objects
	{
		/** \brief
		    Represents the 3D world model as a hash of small voxel
		    blocks
		*/
		template<class TVoxel, class TIndex>
		class ITMScene
		{
		public:
			bool useSwapping;

			/** Scene parameters like voxel size etc. */
			const ITMSceneParams *sceneParams;

			/** Current local content of the 8x8x8 voxel blocks -- stored host or device */
			ITMLocalVBA<TVoxel> localVBA;

			/** Global content of the 8x8x8 voxel blocks -- stored on host only */
			ITMGlobalCache<TVoxel> *globalCache;

			/** Hash table to reference the 8x8x8 blocks */
			TIndex index;

			ITMScene(const ITMSceneParams *sceneParams, bool useSwapping,
				bool allocateGPU)
			 : localVBA(allocateGPU),
			   index(allocateGPU)
			{
				this->sceneParams = sceneParams;
				this->useSwapping = useSwapping;
				if (useSwapping) globalCache = new ITMGlobalCache<TVoxel>();
			}

			~ITMScene(void)
			{
				if (useSwapping) delete globalCache;
			}

			// Suppress the default copy constructor and assignment operator
			ITMScene(const ITMScene&);
			ITMScene& operator=(const ITMScene&);
		};
	}
}
