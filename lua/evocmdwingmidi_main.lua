-- EvoCmdWingMidi v0.2
-- This lua script will poll midi remotes in grandma3 for the name XKeyRotate1-8 and XKeyPress1-8
-- XKeyRotate remotes handle fader control from encoders with MIDI feedback of current fader value
-- XKeyPress remotes target the same sequences but allow manual key configuration (press for toggle/on/off etc..)

-- MIDI Remotes:
--     Main loop will auto create the following MIDI Remotes when not present
--         XKeyRotate1-8: channel 1 cc 6-13 (fader control with feedback)
--         XKeyPress1-8: channel 1 note 6-13 (key presses, manual configuration)
--    The defualt function of pressing an Encoder (XKeyPress1-8) is Toggle, you can manually change each or use the Create Midi Remotes button to batch change them

-- The script will:
--     Update the MidiRemote Targets with the current sequences assigned to the XKeys to keep track of pages changes and moving of sequences
--     Track and send the fader values for Executors 291-298 for syncing encoder values for XKeys 1-8
--     Track and send the status of Executors 191-198 and 291-298 (XKeys 1-16) Tracks: Populated/On/Off/Appearance Color
--     Cache status data to reduce midi traffic on page changes and will send updates when assignments or appearance changes

-- Midi Channels:
--     Channel 1: Fader sync: XKeys 1-8 use CC 6-13 (only on sequence changes/page changes) Manually chaning XKey Encoder values in software will cause Physical Encoders to be out of sync until page/sequence change
--     Channel 2: Status: XKeys 1-16 use CC 1-16 (populated/on/off state)
--     Channel 2: RGB: XKeys 1-16 use CC 17-64 (3 CCs each (rgb), only sent when populated and not black)
--     Channel 3: Page changes: CC 1 = current page number (1-127)

-- Status encoding:
--     Status encoding: 0=not populated, 65=populated+off, 127=populated+on
--     RGB: 0-127 per rgb channel, black sequences sent as white (127,127,127) for visibility
--         You can change the default color variables to replace black appearances
--     Page tracking: Caches state for pages, only sends changes
--     Fader sync: Only sends fader values when sequence assignments change (prevents encoder jumping)

-- XKey RGB
--     You can change the defualt color of newly created XKeys below, it replaces black (can be black for default LEDs off)

-- Many thanks to LenschCode for the original code from Midi Executors v1.1.0
-- GNU GPLv3

local loop
-- Refreshrate in seconds, I spent a lot of time getting the script to be as effecient as possible beause we are using polling, 100ms seems to work great
local rate = 0.1         --100ms

-- Starting CC for MIDI feedback
local startingCC = 6
-- local endingCC = startingCC + 7  -- CC 6-13 for XKeys 1-8
local midiChannel = 1
local statusMidiChannel = 2
local pageMidiChannel = 3

-- Default color for black (0,0,0) sequences - for visibility
local defaultRed = 255
local defaultGreen = 255
local defaultBlue = 255

local running = false
local debugMode = false -- Debug output mode - off by default
local currentPage = nil -- Track current page

-- Page-based state tracking for up to 127 pages
local pageIndex = {} -- Track which pages we've indexed: pageIndex[pageNum] = true
local pageExecutorStates = {} -- [pageNum][execNum] = {populated, on, colorR, colorG, colorB}
local startupComplete = false -- Track if we've done initial indexing

-- Change tracking for exeecutors
local changedExecutors = {} -- List of executors that changed on current page

-- MIDI Remote caching
local cachedXKeyRemotes = {} -- Cache of found XKey remotes: {remote_object, exec_info}
local remoteCacheComplete = false -- Track if we've cached all remotes
local expectedRemoteCount = 16 -- XKeyRotate1-8 + XKeyPress1-8

-- All Executors (Xkeys) to monitor
local EXECUTORS_TO_MONITOR = {191,192,193,194,195,196,197,198,291,292,293,294,295,296,297,298}

-- Current page
local currentCyclePageNum = nil

-- Debug print function - only prints if debug mode is enabled
local function DebugPrint(...)
    if debugMode then
        Printf(...)
    end
end


-- Clear all cached state variables (called on script startup)
local function clearAllCachedState()
    currentPage = nil
    pageIndex = {}
    pageExecutorStates = {}
    startupComplete = false
    changedExecutors = {}
    
    -- Clear MIDI remote cache
    cachedXKeyRemotes = {}
    remoteCacheComplete = false
    
    -- Clear page cache
    currentCyclePageNum = nil
    
    DebugPrint("Cached state cleared - ready for direct access sync")
end


local function getDirectExecutorInfo(execNum)
    -- Get executor status
    local exec, page = GetExecutor(execNum)
    
    if not exec then
        -- Executor doesn't exist
        return {
            faderValue = 0,
            isOn = false,
            isPopulated = false,
            color = {r = 0, g = 0, b = 0},
            object = nil,
            faderRef = nil
        }
    end
    
    -- Get all live values directly from the executor
    local faderValue = exec:GetFader{token = "FaderMaster", faderDisabled = false} or 0
    local myObject = exec.Object
    local isPopulated = (myObject ~= nil)
    local isOn = isPopulated and myObject:HasActivePlayback() or false
    
    -- Get color data
    local color = {r = 0, g = 0, b = 0}
    if isPopulated then
        local apper = myObject["APPEARANCE"]
        if apper then
            color = {
                r = apper['BACKR'] or 0,
                g = apper['BACKG'] or 0,
                b = apper['BACKB'] or 0
            }
        end
    end
    
    -- Get fader reference for MIDI remote assignment
    local faderRef = nil
    local objectListExec = ObjectList("page " .. currentCyclePageNum .. "." .. execNum)[1]
    if objectListExec then
        faderRef = objectListExec.fader
    end
    
    return {
        faderValue = faderValue,
        isOn = isOn,
        isPopulated = isPopulated,
        color = color,
        object = myObject,
        faderRef = faderRef
    }
end


local function getXKeyMapping(remoteName)
    -- Handle both XKeyRotate and XKeyPress remotes
    local rotateNum = remoteName:match("^[Xx][Kk]ey[Rr]otate(%d+)$")
    local pressNum = remoteName:match("^[Xx][Kk]ey[Pp]ress(%d+)$")
    
    local executorNumber = nil
    local remoteType = nil
    
    local keyNum = nil
    if rotateNum then
        keyNum = tonumber(rotateNum)
        executorNumber = 290 + keyNum -- XKeyRotate1 = 291, etc.
        remoteType = "Fader"
    elseif pressNum then
        keyNum = tonumber(pressNum)
        executorNumber = 290 + keyNum -- XKeyPress1 = 291, etc.
        remoteType = "Key"
    end
    
    if executorNumber and (executorNumber >= 291 and executorNumber <= 298) then
        return {
            page = currentCyclePageNum,
            id = executorNumber,
            type = remoteType,
            xkeyNum = keyNum
        }
    else
        return nil
    end
end


local function sendMidiFeedback(ccNumber, value)
    local midiValue = math.floor((value / 100) * 127)
    local command = 'SendMIDI "Control" ' .. midiChannel .. '/' .. ccNumber .. ' ' .. midiValue
    Cmd(command)
    
    coroutine.yield(0.010) -- 10ms delay between midi sends  to not overwhelm teensy buffer
end


local function sendMidiStatus(channel, ccNumber, value)
    local midiValue = value > 127 and 127 or (value < 0 and 0 or math.floor(value))
    local command = 'SendMIDI "Control" ' .. channel .. '/' .. ccNumber .. ' ' .. midiValue
    Cmd(command)
    coroutine.yield(0.010) -- 10ms delay
end

local function sendPageChange(pageNumber)
    local pageValue = pageNumber > 127 and 127 or (pageNumber < 1 and 1 or math.floor(pageNumber))
    sendMidiStatus(pageMidiChannel, 1, pageValue)
    DebugPrint("Page change sent: %d", pageValue)
end


local function sendMidiColor(channel, ccBase, color)
    -- Convert 0-255 color values to 0-127 MIDI values
    local rMidi, gMidi, bMidi
    
    if color.r == 0 and color.g == 0 and color.b == 0 then
        -- If color is black (0,0,0), use default color for visibility
        rMidi = math.floor((defaultRed / 255) * 127)
        gMidi = math.floor((defaultGreen / 255) * 127)
        bMidi = math.floor((defaultBlue / 255) * 127)
    else
        rMidi = math.floor((color.r / 255) * 127)
        gMidi = math.floor((color.g / 255) * 127)
        bMidi = math.floor((color.b / 255) * 127)
    end
    
    sendMidiStatus(channel, ccBase, rMidi)     -- Red
    sendMidiStatus(channel, ccBase + 1, gMidi) -- Green
    sendMidiStatus(channel, ccBase + 2, bMidi) -- Blue
end

-- Get cached executor state for comparison
local function getCachedExecutorState(pageNum, execNum)
    if not pageExecutorStates[pageNum] then
        return nil, nil, nil, nil, nil -- No cached data for this page
    end
    
    local cached = pageExecutorStates[pageNum][execNum]
    if not cached then
        return nil, nil, nil, nil, nil -- No cached data for this executor
    end
    
    return cached.populated, cached.on, cached.colorR, cached.colorG, cached.colorB
end

-- Cache executor state for future comparison
local function setCachedExecutorState(pageNum, execNum, populated, on, colorR, colorG, colorB)
    if not pageExecutorStates[pageNum] then
        pageExecutorStates[pageNum] = {}
    end
    
    pageExecutorStates[pageNum][execNum] = {
        populated = populated,
        on = on,
        colorR = colorR,
        colorG = colorG,
        colorB = colorB,
    }
end

-- Send fader sync for a specific executor (only when sequence assignment changes)
local function sendFaderSync(execNum, xkeyNum, reason)
    local currentPageNum = currentCyclePageNum
    
    -- Only send fader sync for executors 291-298 (XKeys 1-8) (Encoders 6-13)
    if execNum < 291 or execNum > 298 then
        return false
    end
    
    
    -- Get current fader value to sync Teensy encoder tracking
    local currentValue = getDirectExecutorInfo(execNum).faderValue
    local faderCCNumber = startingCC + xkeyNum - 1
    sendMidiFeedback(faderCCNumber, currentValue)
    
    DebugPrint("Executor %d (XKey %d): Fader sync=%d%% (CC:%d) - %s", 
           execNum, xkeyNum, math.floor(currentValue), faderCCNumber, reason)
    
    -- Preserve cached state (no fader state tracking needed)
    local cachedPopulated, cachedOn, cachedColorR, cachedColorG, cachedColorB = getCachedExecutorState(currentPageNum, execNum)
    setCachedExecutorState(currentPageNum, execNum, cachedPopulated, cachedOn, cachedColorR, cachedColorG, cachedColorB)
    
    return true
end

-- Send MIDI status for a specific executor 
local function sendExecutorStatus(execNum, xkeyNum, force)
    local currentPageNum = currentCyclePageNum
    
    -- Get current states
    local execInfo = getDirectExecutorInfo(execNum)
    local isPopulated = execInfo.isPopulated
    local isOn = execInfo.isOn  
    local color = execInfo.color
    
    local populatedState = isPopulated
    local onState = isOn
    local colorR, colorG, colorB = color.r, color.g, color.b
    
    -- Get cached state for comparison
    local cachedPopulated, cachedOn, cachedColorR, cachedColorG, cachedColorB = getCachedExecutorState(currentPageNum, execNum)
    
    local messagesSent = 0
    local statusChanged = force or (cachedPopulated ~= populatedState) or (cachedOn ~= onState)
    local colorChanged = force or (cachedColorR ~= colorR) or (cachedColorG ~= colorG) or (cachedColorB ~= colorB)
    local sequenceChanged = false
    
    -- Check if sequence assignment changed (for fader sync)
    local wasPopulated = cachedPopulated or false
    local becamePopulated = false
    if statusChanged and execNum >= 291 and execNum <= 298 then
        -- For executors 291-298, check if the populated state changed (sequence assignment)
        if wasPopulated ~= populatedState then
            sequenceChanged = true
            if not wasPopulated and populatedState then
                becamePopulated = true
            end
        end
    else
        -- For executors 191-198 (XKeys 9-16), check if became populated (hard-coded, no MIDI remote management)
        if not wasPopulated and populatedState then
            becamePopulated = true
        end
    end
    
    -- Send status CC only if status changed
    if statusChanged then
        local statusValue = 0
        if isPopulated then
            if isOn then
                statusValue = 127  -- Populated and on
            else
                statusValue = 65   -- Populated and off
            end
        else
            statusValue = 0  -- Not populated
        end
        
        sendMidiStatus(statusMidiChannel, xkeyNum, statusValue)
        messagesSent = messagesSent + 1
        
        DebugPrint("Executor %d (XKey %d): Status=%d (Pop=%s On=%s)", 
               execNum, xkeyNum, statusValue,
               isPopulated and "YES" or "NO", isOn and "YES" or "NO")
        
        -- Send fader sync if sequence assignment changed
        if sequenceChanged then
            sendFaderSync(execNum, xkeyNum, "sequence assignment changed")
        end
    end
    
    -- Send RGB if Key is populated AND color changed, OR Key just became populated (force RGB send for new sequences)
    if isPopulated and (colorChanged or becamePopulated) then
        local rgbCCBase = 16 + (xkeyNum - 1) * 3 + 1
        sendMidiColor(statusMidiChannel, rgbCCBase, color)
        messagesSent = messagesSent + 3
        
        if becamePopulated then
            DebugPrint("Executor %d (XKey %d): RGB=(%d,%d,%d) - newly populated", 
                   execNum, xkeyNum, color.r, color.g, color.b)
        else
            DebugPrint("Executor %d (XKey %d): RGB=(%d,%d,%d)", 
                   execNum, xkeyNum, color.r, color.g, color.b)
        end
    elseif not isPopulated and colorChanged then
        DebugPrint("Executor %d (XKey %d): Unpopulated (no RGB sent)", execNum, xkeyNum)
    end
    
    -- Update cached state if anything was sent or changed
    if statusChanged or colorChanged then
        setCachedExecutorState(currentPageNum, execNum, populatedState, onState, colorR, colorG, colorB)
    end
    
    return messagesSent > 0 -- Indicate if we sent anything
end

-- Send full page data (startup or new page indexing)
local function sendFullPageData(pageNum, reason)
    DebugPrint("=== SENDING FULL PAGE DATA: Page %d (%s) ===", pageNum, reason)
    local messagesSent = 0
    
    -- Send fader sync for executors 291-298 to initialize Teensy encoder tracking
    for execNum = 291, 298 do
        local xkeyNum = execNum - 290  -- 1-8
        if sendFaderSync(execNum, xkeyNum, reason) then
            messagesSent = messagesSent + 1
        end
    end
    
    -- Send status data for all 16
    for _, execNum in ipairs(EXECUTORS_TO_MONITOR) do
        if execNum >= 291 and execNum <= 298 then
            -- Executors 291-298 = XKeys 1-8
            local xkeyNum = execNum - 290
            if sendExecutorStatus(execNum, xkeyNum, true) then
                messagesSent = messagesSent + 1
            end
        elseif execNum >= 191 and execNum <= 198 then
            -- Executors 191-198 = XKeys 9-16  
            local xkeyNum = execNum - 190
            local actualXkeyNum = xkeyNum + 8
            if sendExecutorStatus(execNum, actualXkeyNum, true) then
                messagesSent = messagesSent + 1
            end
        end
    end
    
    -- Mark page as indexed
    pageIndex[pageNum] = true
    
    DebugPrint("Full page data sent: %d messages", messagesSent)
end

-- Send changes for current page
local function sendChangedExecutors()
    local messagesSent = 0
    
    local changedExecCount = 0
    for _ in pairs(changedExecutors) do 
        changedExecCount = changedExecCount + 1 
    end
    
    DebugPrint("=== PROCESSING CHANGES ===\nChanged executors: %d", changedExecCount)
    
    -- Process all changed executors immediately
    for execNum, _ in pairs(changedExecutors) do
        if execNum >= 291 and execNum <= 298 then
            -- Executors 291-298 = XKeys 1-8
            local xkeyNum = execNum - 290  -- 1-8
            
            -- Send status data (includes fader sync if sequence changed)
            if sendExecutorStatus(execNum, xkeyNum, false) then
                messagesSent = messagesSent + 1
                DebugPrint("Executor %d (XKey %d) status updated", execNum, xkeyNum)
            end
            
        elseif execNum >= 191 and execNum <= 198 then
            -- Executors 191-198 = XKeys 9-16
            local xkeyNum = execNum - 190  -- 1-8
            local actualXkeyNum = xkeyNum + 8  -- 9-16
            
            if sendExecutorStatus(execNum, actualXkeyNum, false) then
                messagesSent = messagesSent + 1
                DebugPrint("Executor %d (XKey %d) status updated", execNum, actualXkeyNum)
            end
        end
    end
    
    -- Clear changed executors list after processing
    changedExecutors = {}
    
    DebugPrint("Messages sent: %d", messagesSent)
end

local function checkForExecutorChanges()
    local currentPageNum = currentCyclePageNum
    local contentChanged = false
    local stateChanged = false
    local colorChanged = false
    local pageChanged = false
    
    -- Check if page changed
    if currentPage == nil then
        -- First startup
        currentPage = currentPageNum
        pageChanged = true
        
        if not startupComplete then
            DebugPrint("=== STARTUP: Indexing page %d ===", currentPageNum)
            sendPageChange(currentPageNum)
            sendFullPageData(currentPageNum, "startup")
            startupComplete = true
            return -- Exit early after startup
        end
        
    elseif currentPage ~= currentPageNum then
        -- Page actually changed
        DebugPrint("=== PAGE CHANGE: %d → %d ===", currentPage, currentPageNum)
        currentPage = currentPageNum
        pageChanged = true
        
        -- Send page change notification
        sendPageChange(currentPageNum)
        
        -- Check if this page has been indexed before
        if not pageIndex[currentPageNum] then
            -- New page - send full data including fader sync
            DebugPrint("New page detected - sending full data")
            sendFullPageData(currentPageNum, "new page")
            return -- Exit early after full page send
        else
            -- Known page - send fader sync for encoder tracking, then let normal change detection handle differences
            DebugPrint("Returning to known page %d - sending fader sync", currentPageNum)
            for execNum = 291, 298 do
                local xkeyNum = execNum - 290  -- 1-8
                sendFaderSync(execNum, xkeyNum, "known page change")
            end
        end
    end
    
    -- Check for executor state changes
    for _, execNum in ipairs(EXECUTORS_TO_MONITOR) do

            local execInfo = getDirectExecutorInfo(execNum)
            local currentObject = execInfo.object
            local isPopulated = execInfo.isPopulated
            local currentOnState = execInfo.isOn
            local currentColor = execInfo.color
            
            -- Get cached state for comparison 
            local cachedPopulated, cachedOn, cachedColorR, cachedColorG, cachedColorB = getCachedExecutorState(currentPageNum, execNum)
            
            local executorChanged = false
            
            -- Check if status changed from cached state
            if (cachedPopulated ~= isPopulated) or (cachedOn ~= currentOnState) then
                DebugPrint("[CHANGE] Executor %d: Status changed (Populated: %s, On: %s)", 
                       execNum, isPopulated and "true" or "false", currentOnState and "true" or "false")
                executorChanged = true
                contentChanged = true
                stateChanged = true
            end
            
            -- Check if color changed from cached state
            if (cachedColorR ~= currentColor.r) or (cachedColorG ~= currentColor.g) or (cachedColorB ~= currentColor.b) then
                DebugPrint("[CHANGE] Executor %d: Color changed to RGB(%d,%d,%d)", 
                       execNum, currentColor.r, currentColor.g, currentColor.b)
                executorChanged = true
                colorChanged = true
            end
            
            -- Mark executor as changed if any property changed
            if executorChanged then
                changedExecutors[execNum] = true
            end
    end
    
    -- Send feedback if anything changed (but not for page changes - handled above)
    if not pageChanged and (contentChanged or stateChanged or colorChanged) then
        if contentChanged then
            DebugPrint("=== Executor content changes detected ===")
        end
        if stateChanged then
            DebugPrint("=== Executor state changes detected ===")
        end
        if colorChanged then
            DebugPrint("=== Executor color changes detected ===")
        end
        
        -- Small delay for grandMA to finish processing the changes
        coroutine.yield(0.1) -- 100ms delay
        
        DebugPrint("=== Sending MIDI updates ===")
        sendChangedExecutors()
    end
end


-- MIDI REMOTE CREATION --

local function createMidiRemotes(chosenPressKey)
    -- Stop script if running
    if running then
        Printf("EvoCmdWingMidi: Stopping script to create MIDI remotes…")
        running = false
    end

    chosenPressKey = chosenPressKey or "Toggle"

    local midiPool = Root().ShowData.Remotes.MIDIRemotes
    if not midiPool then
        Printf("ERROR: Could not access MIDI Remote pool.")
        return
    end

    -- Create 16 midi remotes
    local desired = {}
    -- Rotate (CC 6–13)
    for i = 1, 8 do
        table.insert(desired, {
            name = ("XKeyRotate%d"):format(i),
            kind = "Control",
            chan = 1,
            cc   = 5 + i  -- 6-13
        })
    end
    -- Press (Note 6–13)
    for i = 1, 8 do
        table.insert(desired, {
            name = ("XKeyPress%d"):format(i),
            kind = "Note",
            chan = 1,
            note = 5 + i, -- 6-13
            keyAction = chosenPressKey
        })
    end

    -- Build a lookup of existing remotes by name
    local existing = {}
    for _, r in pairs(midiPool:Children()) do
        if r.name then
            existing[r.name] = r
        end
    end

    local created = {}
    local updated = {}
    local function setCommonFields(r, spec)
        local addr = r:ToAddr()

        -- tiny helper to avoid repeating Cmd syntax
        local function setProp(propName, value, isString)
            local v = isString and ('"' .. value .. '"') or tostring(value)
            Cmd('Set ' .. addr .. ' Property "' .. propName .. '" ' .. v)
        end

        -- Settings
        setProp("MIDICHANNEL", spec.chan, false)
        Cmd('Set ' .. addr .. ' Property "TARGET" ""')
        setProp("FADER", "", true)

        if spec.kind == "Control" then
            -- XKeyRotate: CC
            Cmd('Set ' .. addr .. ' Property "MIDITYPE" 3')   -- Control
            setProp("MIDIINDEX", spec.cc, false)
            setProp("KEY", "", true)
        else
            -- XKeyPress: Note
            Cmd('Set ' .. addr .. ' Property "MIDITYPE" 0')   -- Note
            setProp("MIDIINDEX", spec.note, false)
            setProp("KEY", spec.keyAction, true)
        end
    end

    for _, spec in ipairs(desired) do
        local r = existing[spec.name]
        if not r then
            -- Append Midi Remotes to end
            r = midiPool:Append()
            local addr = r:ToAddr()
            Cmd('Set ' .. addr .. ' Property "Name" "' .. spec.name .. '"')
            setCommonFields(r, spec)
            table.insert(created, spec.name)
        else
            local addr = r:ToAddr()
            Cmd('Set ' .. addr .. ' Property "Name" "' .. spec.name .. '"')
            setCommonFields(r, spec)
            table.insert(updated, spec.name)
        end
    end

    Printf("=== MIDI Remote Creation Summary ===")
    if #created > 0 then
        Printf("Created (%d):", #created)
        for _, n in ipairs(created) do
            Printf("  %s", n)
        end
    else
        Printf("No new remotes created.")
    end

    if #updated > 0 then
        Printf("Updated (%d):", #updated)
        for _, n in ipairs(updated) do
            Printf("  %s", n)
        end
    end
    Printf("Done. Restart the script (Start) when you’re ready.")
end


local function parseMidiRemotes()
    -- Cache XKey remotes on first run, then only process cached ones
    
    if not remoteCacheComplete then
        -- INITIAL SCAN: Find and cache all XKey remotes
        cachedXKeyRemotes = {}  -- Clear any partial cache
        local foundCount = 0
        
        for key, remote in pairs(Root().ShowData.Remotes.MIDIRemotes:Children()) do
            local name = remote.name
            local nameUpper = name:upper()
            
            if string.match(nameUpper, "^XKEYROTATE%d+$") or string.match(nameUpper, "^XKEYPRESS%d+$") then
                local exec = getXKeyMapping(remote.name)
                if exec ~= nil then
                    -- Cache this remote with its exec info
                    table.insert(cachedXKeyRemotes, {
                        remote = remote,
                        exec = exec,
                        name = remote.name
                    })
                    foundCount = foundCount + 1
                end
            end
        end
        
        if foundCount >= expectedRemoteCount then
            remoteCacheComplete = true
            DebugPrint("MIDI remotes cached: %d/%d found", foundCount, expectedRemoteCount)
        elseif foundCount > 0 then
            DebugPrint("MIDI remotes: %d/%d found (scanning...)", foundCount, expectedRemoteCount)
        else
            -- Missing Midi Remotes create them
            Printf("WARNING: No XKey remotes found - Creating Remote with defualt Toggle for Press")
            createMidiRemotes()

            clearAllCachedState()
            running = true
            Printf("Remotes created. Restarting EvoCmdWingMidi…")
            loop()
            return
        end
    end
    
    -- Process only cached remotes
    if remoteCacheComplete then
        for _, cachedRemote in pairs(cachedXKeyRemotes) do
            local remote = cachedRemote.remote
            local exec = cachedRemote.exec
            
            -- Validate remote still exists
            if remote and remote.name then
                -- Update exec info with current page (page may have changed)
                exec.page = currentCyclePageNum
                
                -- Always update the target object
                local currentObject = getDirectExecutorInfo(exec.id).object
                if remote.target ~= currentObject then
                    remote.target = currentObject
                end
                
                if remote.target == nil then
                    -- No target assigned, clear fader but preserve manual key settings for XKeyPress
                    if exec.type == "Fader" then
                        -- XKeyRotate: clear both key and fader
                        if remote.key ~= "" then
                            remote.key = ""
                        end
                    end
                    -- Always clear fader for both types when no target
                    if remote.fader ~= "" then
                        remote.fader = ""
                    end
                    -- For XKeyPress (exec.type == "Key"), we keep the manual key setting
                else
                    if exec.type == "Fader" then
                        -- XKeyRotate: Handle fader control
                        if remote.key ~= "" then
                            remote.key = ""
                        end
                        local currentFaderRef = getDirectExecutorInfo(exec.id).faderRef
                        if remote.fader ~= currentFaderRef then
                            remote.fader = currentFaderRef
                        end
                    elseif exec.type == "Key" then
                        -- XKeyPress: Only target assignment, leave key action for manual config
                        if remote.fader ~= "" then
                            remote.fader = ""
                        end
                    end
                end
            else
                -- Remote no longer exists - invalidate cache and restart scanning
                DebugPrint("MIDI remote cache invalid - rescanning")
                cachedXKeyRemotes = {}
                remoteCacheComplete = false
                return  -- Exit and let next cycle rebuild cache
            end
        end
    end
end

loop = function()
    while running do
        currentCyclePageNum = CurrentExecPage().no
        
        parseMidiRemotes()
        checkForExecutorChanges()
        coroutine.yield(rate)
    end
end

-- Key actions for XKeyPress remotes
local XKEY_PRESS_ACTIONS = {
    "Empty",
    ">>>",
    "<<<",
    "Black",
    "Call",
    "DoubleSpeed",
    "Flash",
    "Go+",
    "Go-",
    "Goto",
    "HalfSpeed",
    "Kill",
    "LearnSpeed",
    "Load",
    "On",
    "Off",
    "Pause",
    "Rate 1",
    "Select",
    "SelectFixtures",
    "Speed 1",
    "Swap",
    "Temp",
    "Toggle",
    "Top"
}

local function promptXKeyPressKey(defaultKey)
    defaultKey = defaultKey or "Toggle"
    local desc = {
        title = "XKeyPress Default Key",
        caller = GetFocusDisplay(),
        items  = XKEY_PRESS_ACTIONS,
        selectedValue = defaultKey,
        add_args = {FilterSupport="Yes"},
    }
    local idx, val = PopupInput(desc)
    return val
end


local function StartGui(config)
    local debugText = debugMode and "Debug: ON" or "Debug: OFF"
    local descTable = {
        title = "EvoCmdWingMidi v0.2",
        caller = GetFocusDisplay(),
        items = {"Start","Stop", debugText},
        selectedValue = "Start",
        add_args = {FilterSupport="Yes"},
    }

    if running then
        descTable.items = {"Stop", "Create MIDI Remotes", debugText}
    else
        descTable.items = {"Start", "Create MIDI Remotes", debugText}
    end

    local index, name = PopupInput(descTable)
    return index, name;
end

function main()
    local index, name = StartGui()

    if not name then
        return
    end

    if name == "Start" then
        if not running then
            -- Clear all cached state on startup
            clearAllCachedState()
            
            running = true
            Printf("Starting -- EvoCmdWingMidi v0.2...")
            Printf("Monitoring: Executors 191-198 and 291-298")
            Printf("XKeyRotate1-8: MIDI Channel %d, CC %d-%d (with feedback)", midiChannel, startingCC, (startingCC + 7))
            Printf("XKeyPress1-8: MIDI Channel %d, CC %d-%d (no feedback)", midiChannel, startingCC, (startingCC + 7))
            Printf("Status Data on Channel %d :", statusMidiChannel)
            Printf("  Status: XKeys 1-16 use CC 1-16")
            Printf("    0 = Not populated")
            Printf("    65 = Populated and off") 
            Printf("    127 = Populated and on")
            Printf("  RGB: XKeys 1-16 use CC 17-64 (3 CCs each)")
            Printf("Page Changes on Channel %d:", pageMidiChannel)
            Printf("  CC 1 = Current page number (1-127)")
            Printf("  Smart fader sync for XKeys 1-8 (Channel %d, CC %d-%d)", midiChannel, startingCC, (startingCC + 7))
            Printf("  NOTE: Restart this script if you reset the Teensy OR change MIDI remote names to resync!")
            loop()
        end
    elseif name == "Stop" then
        running = false
        Printf("Stopping -- EvoCmdWingMidi v0.2...")
    elseif name == "Create MIDI Remotes" then
        local picked = promptXKeyPressKey("Toggle")
        if picked == nil then
            Printf("Creation canceled.")
            return
        end
        createMidiRemotes(picked)
    elseif name and name:find("Debug:") then
        debugMode = not debugMode
        local status = debugMode and "enabled" or "disabled"
        Printf("Debug mode %s", status)
    end
end

return main