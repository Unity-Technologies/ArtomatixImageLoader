using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Artomatix.ImageLoader.MemoryManagement
{
    public delegate void OnAllocatedCallback(IntPtr memory, ulong size, string description);
    public delegate void OnDeallocatedCallback(IntPtr memory);

    /// <summary>
    /// Tracking proxy class for use by the C#/C++ interop layer to send notifications
    /// about unmanaged memory allocation and deallocation.
    /// </summary>
    public static class Tracker
    {
        /// <summary>
        /// Allocation callback that can be used by a memory tracking component
        /// to listen for allocation events.
        /// </summary>
        public static OnAllocatedCallback AllocHandler { get; set; }

        /// <summary>
        /// Deallocation callback that can be used by a memory tracking component
        /// to listen for deallocation events.
        /// </summary>
        public static OnDeallocatedCallback DeAllocHandler { get; set; }

        /// <summary>
        /// Called by interop code to notify any listeners that memory has been allocated.
        /// </summary>
        /// <param name="memory">Pointer to the allocated memory.</param>
        /// <param name="size">Size of the allocated memory.</param>
        /// <param name="description">
        /// Description of what the memory contains, to help with debugging.
        /// </param>
        public static void TrackAlloc(IntPtr memory, ulong size, string description)
        {
            AllocHandler?.Invoke(memory, size, description);
        }

        /// <summary>
        /// Called by interop code to notify any listeners that memory has been deallocated.
        /// </summary>
        /// <param name="memory">Pointer to the allocated memory.</param>
        public static void TrackFree(IntPtr memory)
        {
            DeAllocHandler?.Invoke(memory);
        }
    }
}
