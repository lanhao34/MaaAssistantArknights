// <copyright file="Assistant.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

namespace MeoAssistant.Core
{
    using MeoAssistant.Core.Tasks;
    using MeoAssistant.Core.Utilities;

    /// <summary>
    /// The API callback of Assistant.
    /// </summary>
    /// <param name="message">The message.</param>
    /// <param name="detailJson">The JSON content.</param>
    /// <param name="customArgs">The custom arguments.</param>
    public delegate void AssistantApiCallback(int message, string detailJson, params object[] customArgs);

    /// <summary>
    /// The Assistant.
    /// </summary>
    public class Assistant : IDisposable
    {
        private bool isInited;

        private Guid guid;

        private Controller controller;

        private RuntimeStatus runtimeStatus;

        private bool isThreadExited;

        private List<KeyValuePair<int, PackageTask>> taskList;

        private int taskId;

        private AssistantApiCallback? callback;

        private object[] callbackArgs;

        private bool isThreadIdle = true;

        private Mutex mutex;

        /* m_condvar */

        private Queue<KeyValuePair<AssistantMessage, object>> messageQueue;

        private Mutex messageMutex;

        /* m_msg_condvar */

        private Task messageThread;

        private Task workingThread;

        private bool disposed;

        /// <summary>
        /// Initializes a new instance of the <see cref="Assistant"/> class.
        /// </summary>
        /// <param name="callback">The callback.</param>
        /// <param name="callbackArg">The callback arguments.</param>
        public Assistant(AssistantApiCallback? callback = null, params object[] callbackArg)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="Assistant"/> class.
        /// </summary>
        ~Assistant()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// Connects to the emulator.
        /// </summary>
        /// <param name="adbPath">The ADB path.</param>
        /// <param name="address">The address.</param>
        /// <param name="config">The config.</param>
        /// <returns>A value that indicates whether the operation is successful.</returns>
        public bool Connect(string adbPath, string address, string config)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Appends task.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="args">The arguments.</param>
        /// <returns>The task ID.</returns>
        public int AppendTask(string type, string args)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Sets task parameters.
        /// </summary>
        /// <param name="taskId">The task ID.</param>
        /// <param name="args">The arguments.</param>
        /// <returns>A value that indicates whether the operation is successful.</returns>
        public bool SetTaskParams(int taskId, string args)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Starts task queue.
        /// </summary>
        /// <param name="isBlocking">A value that indicates whether the operation is blocking.</param>
        /// <returns>A value that indicates whether the operation is successful.</returns>
        public bool Start(bool isBlocking = true)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Stops task queue and empty it.
        /// </summary>
        /// <param name="isBlocking">A value that indicates whether the operation is blocking.</param>
        /// <returns>A value that indicates whether the operation is successful.</returns>
        public bool Stop(bool isBlocking = true)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets image.
        /// </summary>
        /// <returns>The image in byte array.</returns>
        public byte[] GetImage()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Sends clicking event to <see cref="Controller"/>.
        /// </summary>
        /// <param name="x">The X coordinate.</param>
        /// <param name="y">The Y coordinate.</param>
        /// <param name="isBlocking">A value that indicates whether the operation is blocking.</param>
        /// <returns>A value that indicates whether the operation is successful.</returns>
        public bool ControllerClick(int x, int y, bool isBlocking = true)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets GUID.
        /// </summary>
        /// <returns>The GUID.</returns>
        public Guid GetGuid()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the list of tasks.
        /// </summary>
        /// <returns>A list of task IDs.</returns>
        public IEnumerable<int> GetTaskList()
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Disposes the object.
        /// </summary>
        /// <param name="disposing">A value that indicates whether the call is from <see cref="Dispose()"/>.</param>
        protected void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    // Clean up managed sources.
                    // Call Dispose() on your managed sources.
                }

                // Clean up unmanaged sources.
                this.disposed = true;
            }
        }

        private static void TaskCallback(AssistantMessage message, object detail, params object[] customArgs)
        {
            // TODO
        }

        private void WorkingProc()
        {
            // TODO
        }

        private void MessageProc()
        {
            // TOOD
        }

        private void AppendCallback(AssistantMessage message, object detail)
        {
            // TODO
        }

        private void ClearCache()
        {
            // TODO
        }

        private bool Inited()
        {
            // TODO
            return false;
        }
    }
}
