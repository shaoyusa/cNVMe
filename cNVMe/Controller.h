/*
###########################################################################################
// cNVMe - An Open Source NVMe Device Simulation - MIT License
// Copyright 2017 - Intel Corporation

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
// OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
############################################################################################
Controller.h - A header file for the NVMe Controller
*/

#pragma once

#include "Command.h"
#include "ControllerRegisters.h"
#include "Identify.h"
#include "LogPages.h"
#include "Namespace.h"
#include "PCIe.h"
#include "Types.h"
#include "Queue.h"

#define ADMIN_QUEUE_ID 0
#define FIRMWARE_EYE_CATCHER "cNVMe"
#define MAX_COMMAND_IDENTIFIER 0xFFFF
#define MAX_SUBMISSION_QUEUES  0xFFFF

using namespace cnvme;
using namespace cnvme::command;

namespace cnvme { namespace controller { class Controller; } }
typedef void (cnvme::controller::Controller::*NVMeCaller)(NVME_COMMAND&, COMPLETION_QUEUE_ENTRY&);
#define NVME_CALLER_HEADER(commandName) void commandName(NVME_COMMAND& command, COMPLETION_QUEUE_ENTRY& completionQueueEntryToPost)

namespace cnvme
{
	namespace controller
	{

		class Controller
		{
		public:
			/// <summary>
			/// Constructor for the controller
			/// </summary>
			Controller();

			/// <summary>
			/// Destructor for the controller
			/// </summary>
			~Controller();

			/// <summary>
			/// Gets the internal ControllerRegisters
			/// </summary>
			/// <returns>ControllerRegisters object</returns>
			cnvme::controller::registers::ControllerRegisters* getControllerRegisters();

			/// <summary>
			/// Gets the PCI (Bus) Express Registers 
			/// </summary>
			/// <returns></returns>
			pci::PCIExpressRegisters* getPCIExpressRegisters();

			/// <summary>
			/// Function to reset the controller
			/// Called by the ControllerRegisters as a callback	
			/// Note: This should probably be much more private a method. Only the controller registers should call this
			/// </summary>
			void controllerResetCallback();

			/// <summary>
			/// Wait for an iteration of the interrupt loop
			/// </summary>
			void waitForChangeLoop();

			/// <summary>
			/// Sets the CRAPI-F
			/// </summary>
			/// <param name="filePath">path to file</param>
			void setCommandResponseFilePath(const std::string filePath);

		private:

			/// <summary>
			/// The controller registers
			/// </summary>
			controller::registers::ControllerRegisters* ControllerRegisters;
			
			/// <summary>
			/// The PCI (Bus) Express Registers
			/// </summary>
			pci::PCIExpressRegisters* PCIExpressRegisters;

			/// <summary>
			/// Looping thread to watch for doorbell writes.
			/// </summary>
			LoopingThread DoorbellWatcher;

			/// <summary>
			/// Used to keep track of the non-deleted but created submission queues
			/// Vector of queue objects
			/// </summary>
			std::vector<Queue*> ValidSubmissionQueues;

			/// <summary>
			/// Used to keep track of the non-deleted but created completion queues
			/// Vector of queue objects
			/// </summary>
			std::vector<Queue*> ValidCompletionQueues;

			/// <summary>
			/// Used to keep track of CIDs that have been used
			/// </summary>
			std::map<UINT_16, std::set<UINT_16>> SubmissionQueueIdToCommandIdentifiers;

			/// <summary>
			/// Function to be called in loop looking for changes
			/// </summary>
			void checkForChanges();

			/// <summary>
			/// This call will take the command of the given submission queue, process the command and
			/// pass back completion via the completion queue doorbell.
			/// </summary>
			/// <param name="submissionQueue">The internal submission queue object for this command</param>
			void processCommandAndPostCompletion(Queue &submissionQueue);

			/// <summary>
			/// Returns a Queue matching the given id
			/// </summary>
			/// <param name="queues">vector of Queues</param>
			/// <param name="id">The queue id</param>
			/// <returns>Queue</returns>
			Queue *getQueueWithId(std::vector<Queue*> &queues, UINT_16 id);

			/// <summary>
			/// Posts the given completion to the given queue.
			/// Fills in sqid, sqhd, cid.
			/// Also can flip the Phase Tag if needed.
			/// </summary>
			/// <param name="completionQueue">Queue to post to</param>
			/// <param name="completionEntry">Entry to post to the queue</param>
			/// <param name="command">The NVMe Command that is having its completion posted</param>
			void postCompletion(Queue &completionQueue, command::COMPLETION_QUEUE_ENTRY completionEntry, command::NVME_COMMAND* command);

			/// <summary>
			/// Returns true if the command id 
			/// </summary>
			/// <param name="commandId">The command ID we are checking for</param>
			/// <param name="submissionQueueId">The sub queue id the command was sent to</param>
			/// <returns>true if valid, False otherwise.</returns>
			bool isValidCommandIdentifier(UINT_16 commandId, UINT_16 submissionQueueId);

			/// <summary>
			/// Resets the internal identify controller to default values.
			/// </summary>
			void resetIdentifyController();

			/// <summary>
			/// Corresponds with the phase tag in the completion queue entry for a queue
			/// </summary>
			std::map<UINT_16, bool> QueueToPhaseTag;

			/// <summary>
			/// Internal Identify Controller Structure
			/// </summary>
			identify::structures::IDENTIFY_CONTROLLER IdentifyController;

			/// <summary>
			/// Internal map from NSID to active Namespace objects
			/// </summary>
			std::map<UINT_32, ns::Namespace> NamespaceIdToActiveNamespace;

			/// <summary>
			/// Internal map from NSID to inactive Namespace objects
			/// </summary>
			std::map<UINT_32, ns::Namespace> NamespaceIdToInactiveNamespace;

			/// <summary>
			/// Gets a payload in the format of a namespace list (4-bytes per NSID)
			/// </summary>
			/// <param name="namespaceMap">map from nsid to namespace object</param>
			/// <param name="startingNsid">NSID to start with</param>
			/// <param name="completionQueueEntryToPost">CQE for an identify call to get this data</param>
			/// <returns>Payload</returns>
			Payload getNamespaceListFromMap(std::map<UINT_32, ns::Namespace> namespaceMap, UINT_32 startingNsid, COMPLETION_QUEUE_ENTRY& completionQueueEntryToPost);

			/// <summary>
			/// Gets a map of all namespaces (active or inactive)
			/// </summary>
			/// <returns></returns>
			std::map<UINT_32, ns::Namespace> getAllocatedNamespaceMap() const;

			/// <summary>
			/// Will attempt to call the command response api file (CRAPI-F)
			/// </summary>
			/// <param name="nvmeCommand">command sent</param>
			/// <param name="completionQueueEntry">completion entry to post back</param>
			/// <param name="SQID">Submission queue id that sent the command</param>
			/// <returns>true if CRAPI handled the command</returns>
			bool handledByCommandResponseApiFile(NVME_COMMAND& nvmeCommand, COMPLETION_QUEUE_ENTRY& completionQueueEntry, UINT_16 SQID);

			/// <summary>
			/// Updates the running firmware to the one with the given slot
			/// </summary>
			/// <param name="firmwareSlot"></param>
			void replaceRunningFirmwareWithOneInSlot(UINT_8 firmwareSlot);

			/// <summary>
			/// Used to store the data downloaded by Firmware Image Download
			/// </summary>
			std::map<UINT_32, Payload> FirmwareImageDWordOffsetToData;

			/// <summary>
			/// Map from the admin command opcode to the function that processes it
			/// </summary>
			static const std::map<UINT_8, NVMeCaller> AdminCommandCallers;

			/// <summary>
			/// Map from the NVM command opcode to the function that processes it
			/// </summary>
			static const std::map<UINT_8, NVMeCaller> NVMCommandCallers;

			/// <summary>
			/// File to call for CRAPI (Command Response API)
			/// </summary>
			std::string CommandResponseApiFilePath;

			/// <summary>
			/// Holds info for LID=3 / Firmware Slot Info
			/// </summary>
			log_pages::FIRMWARE_SLOT_INFO FirmwareSlotInfo;

			/// <summary>
			/// Handling for the NVMe Identify Command
			/// </summary>
			NVME_CALLER_HEADER(adminIdentify);

			/// <summary>
			/// Handling for the NVMe Create IO Completion Queue Command
			/// </summary>
			NVME_CALLER_HEADER(adminCreateIoCompletionQueue);

			/// <summary>
			/// Handling for the NVMe Create IO Submission Queue Command
			/// </summary>
			NVME_CALLER_HEADER(adminCreateIoSubmissionQueue);

			/// <summary>
			/// Handling for the NVMe Delete IO Completion Queue Command
			/// </summary>
			NVME_CALLER_HEADER(adminDeleteIoCompletionQueue);

			/// <summary>
			/// Handling for the NVMe Delete IO Submission Queue Command
			/// </summary>
			NVME_CALLER_HEADER(adminDeleteIoSubmissionQueue);

			/// <summary>
			/// Handling for the NVMe Firmware Commit Command
			/// </summary>
			NVME_CALLER_HEADER(adminFirmwareCommit);

			/// <summary>
			/// Handling for the NVMe Firmware Image Download Command
			/// </summary>
			NVME_CALLER_HEADER(adminFirmwareImageDownload);

			/// <summary>
			/// Handling for the NVM Format command
			/// </summary>
			NVME_CALLER_HEADER(adminFormatNvm);

			/// <summary>
			/// Handling for the NVMe Keep Alive Command
			/// </summary>
			NVME_CALLER_HEADER(adminKeepAlive);

			/// <summary>
			/// Handling for the NVM Flush command
			/// </summary>
			NVME_CALLER_HEADER(nvmFlush);

			/// <summary>
			/// Handling for the NVM Read command
			/// </summary>
			NVME_CALLER_HEADER(nvmRead);

			/// <summary>
			/// Handling for the NVM Write command
			/// </summary>
			NVME_CALLER_HEADER(nvmWrite);
		};
	}
}
