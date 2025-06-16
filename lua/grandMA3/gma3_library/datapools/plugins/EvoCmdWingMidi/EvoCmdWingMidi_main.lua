-- EvoCmdWingMidi Lua Script
-- This lua script will poll midi remotes in grandma3 for the name XKeyRotate1-8 and XKeyPress1-8
-- XKeyRotate remotes handle fader control with MIDI feedback
-- XKeyPress remotes target the same sequences but allow manual key configuration

-- Add 16 midi remotes:
-- XKeyRotate1-8: channel 1 cc 6-13 (fader control with feedback)
-- XKeyPress1-8: channel 1 note 6-13 (key presses, manual configuration)
-- Example: XKeyRotate7 channel 1 cc 12, XKeyPress7 channel 1 cc 6

-- Midi CC 1-5 used for Midi Encoders (by Pro Plugins) for attributes

-- The script will update the MidiRemote Targets with the current sequences assigned to the XKeys

-- Many thanks to LenschCode for the original code from Midi Executors v1.1.0
-- GNU GPLv3

-- Refreshrate in seconds
local rate = 0.5

-- Starting CC for MIDI feedback
local startingCC = 6
local midiChannel = 1

local running = false
local currentPage = nil -- Track current page
local lastExecutorObjects = {} -- Track what's assigned to each executor

local function getXKeyMapping(remoteName)
    -- Handle both XKeyRotate and XKeyPress remotes
    local rotateNum = remoteName:match("^[Xx][Kk]ey[Rr]otate(%d+)$")
    local pressNum = remoteName:match("^[Xx][Kk]ey[Pp]ress(%d+)$")
    
    local executorNumber = nil
    local remoteType = nil
    
    if rotateNum then
        executorNumber = 290 + tonumber(rotateNum) -- XKeyRotate1 = 291, etc.
        remoteType = "Fader"
    elseif pressNum then
        executorNumber = 290 + tonumber(pressNum) -- XKeyPress1 = 291, etc.
        remoteType = "Key"
    end
    
    if executorNumber and executorNumber >= 291 and executorNumber <= 298 then
        return {
            page = CurrentExecPage().no,
            id = executorNumber,
            type = remoteType
        }
    else
        return nil
    end
end

local function getExecKey(page, id)
    local exec = ObjectList("page " .. page .. "." .. id)[1]
    if exec ~= nil then
        return exec.key
    else
        return nil
    end
end

local function getExecFader(page, id)
    local exec = ObjectList("page " .. page .. "." .. id)[1]
    if exec ~= nil then
        return exec.fader
    else
        return nil
    end
end

local function getExecObject(page, id)
    local exec = ObjectList("page " .. page .. "." .. id)[1]
    if exec ~= nil then
        return exec.object
    else
        return nil
    end
end

local function getCurrentFaderValue(page, id)
    local executors = DataPool().Pages[page]:Children()
    for _, exec in pairs(executors) do
        if exec.No == tonumber(id) then
            local val = exec:GetFader{token = "FaderMaster", faderDisabled = false} or 0
            return val
        end
    end
    return 0
end

local function sendMidiFeedback(ccNumber, value)
    local midiValue = math.floor((value / 100) * 127)
    local command = string.format('SendMIDI "Control" %d/%d %d', midiChannel, ccNumber, midiValue)
    Cmd(command)
    -- Printf("MIDI Feedback: %s", command)
    
    -- Small delay between MIDI messages to ensure proper processing
    coroutine.yield(0.01) -- 10ms delay
end

local function sendPageChangeFeedback()
    -- Get XKeyRotate remotes for MIDI feedback (only faders get feedback)
    local xkeyRotateRemotes = {}
    for key, remote in pairs(Root().ShowData.Remotes.MIDIRemotes:Children()) do
        local name = remote.name
        if string.match(name:lower(), "^xkeyrotate%d+$") then
            table.insert(xkeyRotateRemotes, remote)
        end
    end
    
    table.sort(xkeyRotateRemotes, function(a, b) return a.name < b.name end)
    -- Printf("=== Sending MIDI feedback for %d XKeyRotate remotes ===", #xkeyRotateRemotes)
    
    for _, remote in pairs(xkeyRotateRemotes) do
        local exec = getXKeyMapping(remote.name)
        if exec ~= nil then
            -- Extract the number from the remote name and use it directly for CC calculation
            local rotateNum = remote.name:match("^[Xx][Kk]ey[Rr]otate(%d+)$")
            if rotateNum then
                local ccNumber = startingCC + tonumber(rotateNum) - 1
                -- Always get current value regardless of target state
                local currentValue = getCurrentFaderValue(exec.page, exec.id)
                
                Printf("Sending: %s (Exec %d) -> CC %d, Value: %.1f%% (target: %s)", 
                       remote.name, exec.id, ccNumber, currentValue, 
                        remote.target and "assigned" or "empty")
                sendMidiFeedback(ccNumber, currentValue)
            end
        end
    end
    
    -- Printf("=== Completed MIDI updates for %d remotes ===", #xkeyRotateRemotes)
end

local function checkForExecutorChanges()
    local currentPageNum = CurrentExecPage().no
    local contentChanged = false
    local pageChanged = false
    
    -- Check if page changed
    if currentPage == nil then
        currentPage = currentPageNum
        pageChanged = true
        -- Printf("Initial page: %d", currentPage)
    elseif currentPage ~= currentPageNum then
        -- Printf("Page changed from %d to %d", currentPage, currentPageNum)
        currentPage = currentPageNum
        pageChanged = true
    end
    
    -- Check if executor content changed (291-298)
    for execNum = 291, 298 do
        local currentObject = getExecObject(currentPageNum, execNum)
        local objectId = currentObject and tostring(currentObject) or "empty"
        
        if lastExecutorObjects[execNum] == nil then
            -- First time checking this executor
            lastExecutorObjects[execNum] = objectId
            contentChanged = true
        elseif lastExecutorObjects[execNum] ~= objectId then
            -- Content changed
            -- Printf("Executor %d content changed: %s -> %s", execNum, lastExecutorObjects[execNum], objectId)
            lastExecutorObjects[execNum] = objectId
            contentChanged = true
        end
    end
    
    -- Send feedback if page changed OR content changed
    if pageChanged or contentChanged then
        if pageChanged then
            -- Printf("=== Page change detected ===")
        end
        if contentChanged then
            -- Printf("=== Executor content changes detected ===")
        end
        
        -- Wait a moment for grandMA to finish processing the changes
        coroutine.yield(0.1) -- 100ms delay
        
        -- Printf("=== Sending MIDI updates for XKeyRotate remotes ===")
        sendPageChangeFeedback()
    end
end

local function parseMidiRemotes()
    -- Process remotes that start with "XKeyRotate" or "XKeyPress"
    for key, remote in pairs(Root().ShowData.Remotes.MIDIRemotes:Children()) do
        local name = remote.name
        local nameUpper = name:upper()
        
        if string.match(nameUpper, "^XKEYROTATE%d+$") or string.match(nameUpper, "^XKEYPRESS%d+$") then
            local exec = getXKeyMapping(remote.name)
            if exec ~= nil then
                -- Always update the target object
                if remote.target ~= getExecObject(exec.page, exec.id) then
                    remote.target = getExecObject(exec.page, exec.id)
                    -- Printf("Mapped %s to Executor %d", remote.name, exec.id)
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
                        if remote.fader ~= getExecFader(exec.page, exec.id) then
                            remote.fader = getExecFader(exec.page, exec.id)
                        end
                    elseif exec.type == "Key" then
                        -- XKeyPress: Only target assignment, leave key action for manual config
                        if remote.fader ~= "" then
                            remote.fader = ""
                        end
                    end
                end
            end
        end
    end
end

local function loop()
    while running do
        parseMidiRemotes()
        checkForExecutorChanges()
        coroutine.yield(rate)
    end
end

local function StartGui(config)
    local descTable = {
        title = "EvoCmdWingMidi",
        caller = GetFocusDisplay(),
        items = {"Start","Stop","Run Once"},
        selectedValue = "Start",
        add_args = {FilterSupport="Yes"},
    }
    if running then
        descTable.items = {"Stop"}
    else
        descTable.items = {"Start","Run Once"}
    end

    local index, name = PopupInput(descTable)
    return index, name;
end

function main()
    local index, name = StartGui()

    if name == "Start" then
        if not running then
            running = true
            Printf("Starting -- EvoCmdWingMidi...")
            Printf("Monitoring: Executors 291-298")
            Printf("XKeyRotate1-8: MIDI Channel %d, CC %d-%d (with feedback)", midiChannel, startingCC, startingCC + 7)
            Printf("XKeyPress1-8: MIDI Channel %d, CC %d-%d (no feedback)", midiChannel, startingCC, startingCC + 7)
            loop()
        end
    elseif name == "Stop" then
        running = false
        Printf("Stopping -- EvoCmdWingMidi...")
    elseif name == "Run Once" then
        parseMidiRemotes()
        checkForExecutorChanges()
        Printf("Ran XKey MIDI assignment and change check once")
    end
end

return main