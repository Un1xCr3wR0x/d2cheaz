//c++17
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <iostream>

using T_ConMsg = void(*)(const char* fmt, ...);
T_ConMsg _ConMsg = nullptr;
template<class... Types>
void CMSG(const char* fmt, Types... Args)
{
    if (_ConMsg)
        _ConMsg(fmt, Args...);
}

using T_CreateInterface = void* (*)(const char* name, int* out_status);
void* CreateInterface(T_CreateInterface function, const char* name)
{
    int out_status{};
    const auto result = function(name, &out_status);
    if (out_status == 0)
        return result;
    return nullptr;
}

bool fuzzy_memcmp(const std::uint8_t* lhs, const std::uint8_t* rhs, std::size_t size, const char* masks) noexcept
{
    constexpr auto wildcard = '?';
    const auto end = lhs + size;
    for (; lhs < end; ++lhs, ++rhs, ++masks)
    {
        if (*masks != wildcard && *lhs != *rhs)
            return false;
    }
    return true;
}

//maybe shit, not really tested
const std::uint8_t* sigscan_naive(const std::uint8_t* base, std::size_t input_size, const uint8_t* pattern,
    std::size_t pattern_size, const char* masks) noexcept
{
    if (
        pattern_size
        && (input_size >= pattern_size)
        && base
        && pattern
        && masks
        )
    {
        const auto alignmentCount = (input_size - pattern_size) + 1;
        const auto end = base + alignmentCount;
        for (auto current = base; current < end; ++current)
        {
            if (fuzzy_memcmp(current, pattern, pattern_size, masks))
                return current;
        }
    }

    return nullptr;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        FILE* file;
        freopen_s(&file, "CONOUT$", "w", stdout);
        while (!GetAsyncKeyState(VK_END)) {
            if (GetAsyncKeyState(VK_INSERT) & 1) {
                if (const auto tier0 = GetModuleHandleA("tier0.dll"); tier0)
                {
                    if (const auto ConMsgExport = GetProcAddress(tier0, "?ConMsg@@YAXPEBDZZ"); ConMsgExport)
                    {
                        _ConMsg = reinterpret_cast<T_ConMsg>(ConMsgExport);
                        CMSG("Hello world!\n");
                        std::cout << "Hello world!\n";

                        if (const auto client_dll = GetModuleHandleA("client.dll"); client_dll)
                        {
                            MODULEINFO out_modinfo{};
                            if (!K32GetModuleInformation(GetCurrentProcess(), client_dll, &out_modinfo, sizeof(MODULEINFO)))
                            {
                                CMSG("K32GetModuleInformation fail: %u\n", GetLastError());
                                std::cout << "K32GetModuleInformation fail: " << GetLastError() << "\n";
                            }
                            else
                            {
                                const char masks[]{ "xxx????xx????xxx????xxx" };
                                const auto entitysystem_xref =
                                    sigscan_naive((const std::uint8_t*)client_dll, out_modinfo.SizeOfImage,
                                        (const std::uint8_t*)"\x48\x8d\x0d????\xff\x15????\x48\x8b\x0d????\x33\xd2\xe8",
                                        std::size(masks) - 1,
                                        masks);
                                if (!entitysystem_xref)
                                {
                                    CMSG("entitysystem_xref not found!\n");
                                    std::cout << "entitysystem_xref not found!\n";
                                }
                                else
                                {
                                    const auto mov_insn_ptr = entitysystem_xref + 0xD;
                                    const auto rel32 = *(std::int32_t*)(mov_insn_ptr + 0x3);
                                    const auto entity_system_ptr = (void**)(mov_insn_ptr + 0x7 + rel32);
                                    const auto entitysystem = *entity_system_ptr;
                                    if (!entitysystem)
                                    {
                                        CMSG("entitysystem not created yet\n");
                                        std::cout << "entitysystem not created yet\n";
                                    }
                                    else
                                    {
                                        CMSG("entitysystem: 0x%p\n", entitysystem);
                                        std::cout << "entitysystem: " << entitysystem << "\n";
                                        if (const auto engine_dll = GetModuleHandleA("engine2.dll"); engine_dll)
                                        {
                                            if (const auto Engine_CIProc = GetProcAddress(engine_dll, "CreateInterface"); Engine_CIProc)
                                            {
                                                const auto engineclient
                                                    = CreateInterface(reinterpret_cast<T_CreateInterface>(Engine_CIProc),
                                                        "Source2EngineToClient001");
                                                if (engineclient)
                                                {
                                                    CMSG("CEngineClient: 0x%p\n", engineclient);
                                                    std::cout << "CEngineClient: " << engineclient << "\n";

                                                    if (const auto engine_vftable = *reinterpret_cast<void***>(engineclient);
                                                        engine_vftable)
                                                    {
                                                        using T_GetLocalPlayerID = void(*)(void* self_engineclient, int* out_result, int splitscreenslot);
                                                        const auto GetLocalPlayerID
                                                            = reinterpret_cast<T_GetLocalPlayerID>(engine_vftable[29]);
                                                        if (GetLocalPlayerID)
                                                        {
                                                            int playerID = 0;
                                                            GetLocalPlayerID(engineclient, &playerID, 0);
                                                            if (playerID >= 0)
                                                            {
                                                                const auto player_controller_entity_ID = playerID + 1;
                                                                CMSG("Local player id: %d\n", playerID);
                                                                CMSG("Local player entity id: %d\n", player_controller_entity_ID);
                                                                std::cout << "Local player id: " << playerID << "\n";
                                                                std::cout << "Local player entity id: " << player_controller_entity_ID << "\n";

                                                                constexpr auto ENTITY_SYSTEM_LIST_SIZE = 512;
                                                                constexpr auto ENTITY_SYSTEM_LIST_COUNT = 64;
                                                                constexpr auto indexMask = (ENTITY_SYSTEM_LIST_COUNT * ENTITY_SYSTEM_LIST_SIZE) - 1;
                                                                const auto _GetEntityByIndex = [entitysystem](std::size_t index) -> void*
                                                                    {
                                                                        const auto entitysystem_lists = (const void**)((char*)entitysystem + 0x10);
                                                                        const auto list =
                                                                            entitysystem_lists[index / ENTITY_SYSTEM_LIST_SIZE];
                                                                        if (list)
                                                                        {
                                                                            const auto entry_index = index % ENTITY_SYSTEM_LIST_SIZE;
                                                                            constexpr auto sizeof_CEntityIdentity = 0x78;
                                                                            const auto identity = (char*)list + entry_index * sizeof_CEntityIdentity;
                                                                            if (identity)
                                                                            {
                                                                                return *(void**)identity;
                                                                            }
                                                                        }
                                                                        return nullptr;
                                                                    };
                                                                const auto local_player_controller = _GetEntityByIndex(player_controller_entity_ID);
                                                                CMSG("local_player_controller: 0x%p\n", local_player_controller);
                                                                std::cout << "local_player_controller: " << local_player_controller << "\n";
                                                                if (local_player_controller)
                                                                {
                                                                    constexpr auto offset_m_hAssignedHero = 0x7d4;

                                                                    const auto handle_m_hAssignedHero = *(std::uint32_t*)
                                                                        ((char*)local_player_controller + offset_m_hAssignedHero);
                                                                    const auto index_m_hAssignedHero = handle_m_hAssignedHero & indexMask;
                                                                    CMSG("index_m_hAssignedHero: %u\n", index_m_hAssignedHero);
                                                                    std::cout << "index_m_hAssignedHero: " << index_m_hAssignedHero << "\n";
                                                                    const auto local_hero = _GetEntityByIndex(index_m_hAssignedHero);
                                                                    CMSG("local_hero: 0x%p\n", local_hero);
                                                                    std::cout << "local_hero: " << local_hero << "\n";
                                                                    if (local_hero)
                                                                    {
                                                                        constexpr auto offset_m_iHealth = 0x324;
                                                                        CMSG("hp: %d\n", *(int*)((char*)local_hero + offset_m_iHealth));
                                                                        std::cout << "hp: " << *(int*)((char*)local_hero + offset_m_iHealth) << "\n";
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Sleep(200);
        }
        if (file)
            fclose(file);

        FreeConsole();
        
    }

    return TRUE;
}