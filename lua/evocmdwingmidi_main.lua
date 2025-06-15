-- EvoCmdWingMidi Lua Script
-- This lua script will poll midi remotes in grandma3 for the name XKey1-8, and assign the
-- the correct sequence to the midi input, and send midi feedback to keep encoder values matching onpc values

-- Add 8 midi remotes name them XKey1 - XKey8
-- assign a midi channel of 1 and control change from 6 - 13
-- example: XKey7 channel 1 cc 12
-- the script will auto populate the target as sequences change

-- Many thanks to LenschCode for the original code from Midi Executors v1.1.0
-- GNU GPLv3

-- Refreshrate in seconds
local rate = 0.5

-- Starting CC for MIDI feedback
-- First remote = CC 6, second = CC 7, etc. on Channel 1
local startingCC = 6
local midiChannel = 1

local running = false
local currentPage = nil -- Track current page
local lastExecutorObjects = {} -- Track what's assigned to each executor

local function getXKeyMapping(remoteName)
    -- Direct hardcoded mapping: XKey1/Xkey1 = Executor 291, etc.
    local normalizedName = remoteName:gsub("^[Xx]key(%d+)$", "XKey%1")
    
    local xkeyMappings = {
        ["XKey1"] = 291,
        ["XKey2"] = 292,
        ["XKey3"] = 293,
        ["XKey4"] = 294,
        ["XKey5"] = 295,
        ["XKey6"] = 296,
        ["XKey7"] = 297,
        ["XKey8"] = 298
    }
    
    local executorNumber = xkeyMappings[normalizedName]
    if executorNumber then
        return {
            page = CurrentExecPage().no,
            id = executorNumber,
            type = "Fader"
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
    local ccCounter = 0
    
    -- Get remotes that start with "XKey"
    local xkeyRemotes = {}
    for key, remote in pairs(Root().ShowData.Remotes.MIDIRemotes:Children()) do
        local name = remote.name
        if string.sub(name:lower(), 1, 4) == "xkey" then
            table.insert(xkeyRemotes, remote)
        end
    end
    
    table.sort(xkeyRemotes, function(a, b) return a.name < b.name end)
    -- Printf("=== Sending MIDI feedback for %d XKey remotes ===", #xkeyRemotes)
    
    for _, remote in pairs(xkeyRemotes) do
        local exec = getXKeyMapping(remote.name)
        if exec ~= nil then
            ccCounter = ccCounter + 1
            local ccNumber = startingCC + ccCounter - 1
            -- Always get current value regardless of target state
            local currentValue = getCurrentFaderValue(exec.page, exec.id)
            
            -- Printf("Sending: %s (Exec %d) -> CC %d, Value: %.1f%% (target: %s)", 
            --        remote.name, exec.id, ccNumber, currentValue, 
            --        remote.target and "assigned" or "empty")
            sendMidiFeedback(ccNumber, currentValue)
        end
    end
    
    -- Printf("=== Completed MIDI updates for %d remotes ===", ccCounter)
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
        
        -- Printf("=== Sending MIDI updates for all XKey remotes ===")
        sendPageChangeFeedback()
    end
end

local function parseMidiRemotes()
    -- Process remotes that start with "XKey"
    for key, remote in pairs(Root().ShowData.Remotes.MIDIRemotes:Children()) do
        local name = remote.name
        if string.sub(name:lower(), 1, 4) == "xkey" then
            local exec = getXKeyMapping(remote.name)
            if exec ~= nil then
                if remote.target ~= getExecObject(exec.page, exec.id) then
                    remote.target = getExecObject(exec.page, exec.id)
                    -- Printf("Mapped %s to Executor %d", remote.name, exec.id)
                end
                if remote.target == nil then
                    if remote.key ~= "" then
                        remote.key = ""
                    end
                    if remote.fader ~= "" then
                        remote.fader = ""
                    end
                else
                    -- Always assign fader since XKey remotes are faders
                    if remote.key ~= "" then
                        remote.key = ""
                    end
                    if remote.fader ~= getExecFader(exec.page, exec.id) then
                        remote.fader = getExecFader(exec.page, exec.id)
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
            Printf("Monitoring: Executors 291-298, MIDI Channel %d, Starting CC %d", midiChannel, startingCC)
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