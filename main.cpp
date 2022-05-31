#include "fairy_tail.hpp"

#if defined(__linux__)
    #include <unistd.h>
    #define sleepms(milliseconds) usleep(milliseconds * 1000)
#elif defined(_WIN32)
    #include <Windows.h>
    #define sleepms(milliseconds) Sleep(milliseconds)
#endif

#include <cstdlib>
#include <string>
#include <deque>

class CharData {
public:
    static const int mapcenterx = 10;
    static const int mapcentery = 10;
    static const int maplenx = mapcenterx + 1 + mapcenterx; // center + both sides (10)
    static const int mapleny = mapcentery + 1 + mapcentery;
    char map[maplenx*mapleny]; // '?' -- unknown, '@' (also can be '&') -- start point, '#' -- wall, ' ' -- empty, '.' -- empty, we were here
    int posx = 0;
    int posy = 0;
    std::string path = "";
    int countOfExploredBlocks = 0;
    int countOfUnexploredBlocks = 0;

    char outsideBlock = 'O';

    char* at(int x, int y) {
        if (x < -mapcenterx || x > mapcenterx || y < -mapcentery || y > mapcentery) // normally would not happen
            return &outsideBlock;
        return &map[maplenx * (mapcentery+y) + (mapcenterx+x)];
    }

    Direction go_to(Direction d, bool updatePath = true) {
        switch (d) {
            case Direction::Up:
                posy -= 1;
                if (updatePath) path += 'u';
                break;
            case Direction::Down:
                posy += 1;
                if (updatePath) path += 'd';
                break;
            case Direction::Left:
                posx -= 1;
                if (updatePath) path += 'l';
                break;
            case Direction::Right:
                posx += 1;
                if (updatePath) path += 'r';
                break;
            default:
                return d;
        }
        char* curBlock;
        if (*(curBlock = at(posx, posy)) == ' ') *curBlock = '.';
        return d;
    }

    Direction go_back() {
        if (path.length() == 0) // normally would not happen
            return Direction::Pass;

        char lastMove = path.back();
        path.pop_back();
        switch (lastMove) {
            case 'u':
                return go_to(Direction::Down, false);
            case 'd':
                return go_to(Direction::Up, false);
            case 'l':
                return go_to(Direction::Right, false);
            case 'r':
                return go_to(Direction::Left, false);
            default: // just for compiler...
                return Direction::Pass;
        }
    }

    CharData() {
        for (int i = 0; i < maplenx*mapleny; ++i) map[i] = '?';
        *at(0, 0) = '@';
    }
};

void getEnvData(Fairyland& world, Character ch, CharData& chardata) {
    char* curBlock;
    if (*(curBlock = chardata.at(chardata.posx, chardata.posy - 1)) == '?') {
        *curBlock = (world.canGo(ch, Direction::Up) ? ' ' : '#');
        if (*curBlock == ' ') ++chardata.countOfUnexploredBlocks;
    }
    if (*(curBlock = chardata.at(chardata.posx, chardata.posy + 1)) == '?') {
        *curBlock = (world.canGo(ch, Direction::Down) ? ' ' : '#');
        if (*curBlock == ' ') ++chardata.countOfUnexploredBlocks;
    }
    if (*(curBlock = chardata.at(chardata.posx - 1, chardata.posy)) == '?') {
        *curBlock = (world.canGo(ch, Direction::Left) ? ' ' : '#');
        if (*curBlock == ' ') ++chardata.countOfUnexploredBlocks;
    }
    if (*(curBlock = chardata.at(chardata.posx + 1, chardata.posy)) == '?') {
        *curBlock = (world.canGo(ch, Direction::Right) ? ' ' : '#');
        if (*curBlock == ' ') ++chardata.countOfUnexploredBlocks;
    }
}

char* map = nullptr;

void printMaps(CharData& ivan, CharData& elena) {
    std::cout << "===========================================" << std::endl;

    std::string istatus;
    std::string estatus;
    if (map) {
        istatus = estatus = "METSPT";
    } else {
        istatus = (ivan.countOfUnexploredBlocks ? "EXPLOR" : " WAIT ");
        estatus = (elena.countOfUnexploredBlocks ? "EXPLOR" : " WAIT ");
    }

    std::cout << "Ivan's map [" << istatus << "]  | Elena's map [" << estatus << ']' << std::endl;
    for (int y = 0; y < CharData::mapleny; ++y) {
        for (int x = 0; x < CharData::maplenx; ++x) {
            if (x == ivan.posx+CharData::mapcenterx && y == ivan.posy+CharData::mapcentery)
                std::cout << '*';
            else
                std::cout << ivan.map[CharData::maplenx*y+x];
        }
        std::cout << ' ';
        for (int x = 0; x < CharData::maplenx; ++x) {
            if (x == elena.posx+CharData::mapcenterx && y == elena.posy+CharData::mapcentery)
                std::cout << '*';
            else {
                if (elena.map[CharData::maplenx*y+x] == '@') std::cout << '&';
                else std::cout << elena.map[CharData::maplenx*y+x];
            }
        }
        std::cout << std::endl;
    }
}

Direction getDirToUnexplored(CharData& chardata) {
    Direction ret = Direction::Pass;
    if (*chardata.at(chardata.posx, chardata.posy+1) == ' ') {
        ret = Direction::Down;
    } else if (*chardata.at(chardata.posx+1, chardata.posy) == ' ') {
        ret = Direction::Right;
    } else if (*chardata.at(chardata.posx, chardata.posy-1) == ' ') {
        ret = Direction::Up;
    } else if (*chardata.at(chardata.posx-1, chardata.posy) == ' ') {
        ret = Direction::Left;
    }
    return ret;
}

Direction doCharTurn(CharData& chardata) {
    Direction dir = getDirToUnexplored(chardata);
    if (dir != Direction::Pass) {
        --chardata.countOfUnexploredBlocks;
        ++chardata.countOfExploredBlocks;
        return chardata.go_to(dir);
    } else if (chardata.countOfUnexploredBlocks) {
        return chardata.go_back();
    } else return dir;
}

void syncMaps(CharData& syncTo, CharData& syncFrom) {
    int deltaX = syncFrom.posx-syncTo.posx,
        deltaY = syncFrom.posy-syncTo.posy;

    // Since we have bigger map than real so we will "cut" it to avoid index getting out of range
    int y;
    if (deltaY < 0) y = -deltaY;
    else y = 0;
    int maxY;
    if (deltaY > 0) maxY = CharData::mapleny - deltaY;
    else maxY = CharData::mapleny;
    //
    int x;
    int maxX;
    if (deltaX > 0) maxX = CharData::maplenx - deltaX;
    else maxX = CharData::maplenx;

    while (y < maxY) {
        if (deltaX < 0) x = -deltaX;
        else x = 0;
        while (x < maxX) {
            if (syncTo.map[CharData::maplenx*y + x] == '?')
                syncTo.map[CharData::maplenx*y + x] = syncFrom.map[CharData::maplenx*(y+deltaY) + x+deltaX];
            ++x;
        }
        ++y;
    }
}

void printMap(CharData& chardata) {
    int minX = CharData::maplenx, minY = CharData::mapleny, maxX = -1, maxY = -1;
    for (int y = 0; y < CharData::mapleny; ++y) {
        for (int x = 0; x < CharData::maplenx; ++x) {
            if (chardata.map[y*CharData::maplenx+x] != '?') {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    if (minX <= 1) {
        minX = 1;
        maxX = minX + (CharData::mapcenterx - 1);
    }
    if (maxX >= CharData::maplenx - 2) {
        maxX = CharData::maplenx - 2;
        minX = maxX - (CharData::mapcenterx - 1);
    }
    if (minY <= 1) {
        minY = 1;
        maxY = minY + (CharData::mapcentery - 1);
    }
    if (maxY >= CharData::mapleny - 2) {
        maxY = CharData::mapleny - 2;
        minY = maxY - (CharData::mapcentery - 1);
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            char c = chardata.map[y*CharData::maplenx+x];
            if (c == ' ') c = '.';
            std::cout << c;
        }
        std::cout << std::endl;
    }
}

int ix, iy, ex, ey, mx, my, pathlen, meetingspot_x, meetingspot_y,
    istartx, istarty, estartx, estarty; // for ease of realisation

typedef struct {
    int x;
    int y;
} vec2;

bool isInMapBounds(int x, int y) {
    return x >= 0 && y >= 0 && x < mx && y < my;
}

bool tryToAdd(std::deque<vec2>& dq, int x, int y, char val) {
    if (!isInMapBounds(x, y)) return false;
    if (map[mx*y+x] != 0) return false;
    vec2 v;
    v.x = x;
    v.y = y;
    dq.push_back(v);
    map[mx*y+x] = val;
    return true;
}

bool tryToRegisterAsPath(int x, int y, int* origin_x, int* origin_y) {
    if (!isInMapBounds(x, y)) return false;
    if (map[mx*y+x] != map[mx*(*origin_y)+(*origin_x)] - 1) return false;
    map[mx*(*origin_y)+(*origin_x)] = -126; // put path's symbol
    *origin_x = x;
    *origin_y = y;
    return true;
}

bool tryToMoveOnPath(int x, int y, int* origin_x, int* origin_y) {
    if (isInMapBounds(x, y) && map[mx*y+x] == -126) {
        map[mx*(*origin_y)+(*origin_x)] = -125;
        *origin_x = x;
        *origin_y = y;
        return true;
    }
    return false;
}

Direction getNextStep(int& charposx, int& charposy) {
    Direction ret;
    if (charposx == meetingspot_x && charposy == meetingspot_y)
        ret = Direction::Pass;
    else {
        if (tryToMoveOnPath(charposx-1, charposy, &charposx, &charposy)) ret = Direction::Left;
        else if (tryToMoveOnPath(charposx+1, charposy, &charposx, &charposy)) ret = Direction::Right;
        else if (tryToMoveOnPath(charposx, charposy-1, &charposx, &charposy)) ret = Direction::Up;
        else if (tryToMoveOnPath(charposx, charposy+1, &charposx, &charposy)) ret = Direction::Down;
    }
    return ret;
}

int main() {
    std::cout << "Program launched. By the way, final map can be smaller than 10x10 because sometimes we can't just say where exactly explored fragment located. Or it can have 11 size x and/or y because it's impossible in almost all cases to determine what side to cut." << std::endl
    << "Also: program shows count of used turns just after characters' meeting but there could be used extra 1-4 turns for determination of their relative positions." << std::endl << std::endl;

    std::cout << "There are some options (they're kinda additional so I hope that their fails (if any?) will not decrease project's points...):" << std::endl << std::endl;
    std::string choice;

    std::cout << "== Do you want to see visualized progress? ==" << std::endl << std::endl
    << "On every turn maps will be displayed with short delay." << std::endl
    << "NOTICE: since it's an additional feature maps will differ from final variant (by syntax)." << std::endl << std::endl
    << "Meanings of statuses during visualization:" << std::endl
    << "[EXPLOR] - character is exploring an area" << std::endl
    << "[ WAIT ] - character is waiting for other to calculate meeting spot" << std::endl
    << "[METSPT] - character is going to meeting spot" << std::endl << std::endl
    << "Your choice ('y' or 'n'; default value is 'n'): ";
    getline(std::cin, choice);
    bool visualizationRequired = (choice.length() == 1 && choice[0] == 'y');
    std::cout << std::endl;

    std::cout << "== Do you want to explore all area? ==" << std::endl
    << "Obviously, this most-likely will take more turns." << std::endl
    << "Your choice ('y' or 'n'; default value is 'n'): ";
    getline(std::cin, choice);
    bool exploreEverything = (choice.length() == 1 && choice[0] == 'y');

    Fairyland world;
    CharData ivan;
    CharData elena;

    while (true) {
        getEnvData(world, Character::Ivan, ivan);
        getEnvData(world, Character::Elena, elena);

        if (visualizationRequired) {
            std::cout << "\x1b[2J";
            printMaps(ivan, elena);
            sleepms(100);
        }

        bool found;
        if (ivan.countOfUnexploredBlocks == 0 && elena.countOfUnexploredBlocks == 0) {
            if (!map) {
                int ivan_map_xs = 1000, ivan_map_xe = -1000,
                    ivan_map_ys = 1000, ivan_map_ye = -1000,
                    elena_map_xs = 1000, elena_map_xe = -1000,
                    elena_map_ys = 1000, elena_map_ye = -1000;
                for (int y = 0; y < CharData::mapleny; ++y) {
                    for (int x = 0; x < CharData::maplenx; ++x) {
                        if (ivan.map[CharData::maplenx*y+x] != '?') {
                            if (x < ivan_map_xs) ivan_map_xs = x;
                            if (x > ivan_map_xe) ivan_map_xe = x;
                            if (y < ivan_map_ys) ivan_map_ys = y;
                            if (y > ivan_map_ye) ivan_map_ye = y;
                        }
                        if (elena.map[CharData::maplenx*y+x] != '?') {
                            if (x < elena_map_xs) elena_map_xs = x;
                            if (x > elena_map_xe) elena_map_xe = x;
                            if (y < elena_map_ys) elena_map_ys = y;
                            if (y > elena_map_ye) elena_map_ye = y;
                        }
                    }
                }
                if (ivan_map_xe - ivan_map_xs != elena_map_xe - elena_map_xs || ivan_map_ye - ivan_map_ys != elena_map_ye - elena_map_ys) {
                    std::cout << "They will never meet. They're in different \"rooms\" (one room is smaller than other by size). Turns used: " << world.getTurnCount() << "." << std::endl;
                    printMaps(ivan, elena);
                    return 0;
                }
                mx = ivan_map_xe - ivan_map_xs + 1;
                my = ivan_map_ye - ivan_map_ys + 1;
                map = new char[mx*my];
                for (int y = 0; y < my; ++y) {
                    for (int x = 0; x < mx; ++x) {
                        char fst = ivan.map[CharData::maplenx*(ivan_map_ys+y)+(ivan_map_xs+x)],
                            snd = elena.map[CharData::maplenx*(elena_map_ys+y)+(elena_map_xs+x)];
                        if (fst != '?' && fst != '#') fst = ' ';
                        if (snd != '?' && snd != '#') snd = ' ';
                        if (fst != snd) {
                            std::cout << "They will never meet. They're in different \"rooms\" (volume and size are same though). Turns used: " << world.getTurnCount() << "." << std::endl;
                            printMaps(ivan, elena);
                            return 0;
                        }
                        if (fst == '?') fst = -128;
                        else if (fst == '#') fst = -127;
                        else fst = 0;
                        map[mx*y+x] = fst;
                    }
                }
                ix = ivan.posx+CharData::mapcenterx-ivan_map_xs;
                iy = ivan.posy+CharData::mapcentery-ivan_map_ys;
                ex = elena.posx+CharData::mapcenterx-elena_map_xs;
                ey = elena.posy+CharData::mapcentery-elena_map_ys;
                istartx = CharData::mapcenterx-ivan_map_xs;
                istarty = CharData::mapcentery-ivan_map_ys;
                estartx = CharData::mapcenterx-elena_map_xs;
                estarty = CharData::mapcentery-elena_map_ys;

                std::deque<vec2> queue;
                tryToAdd(queue, ix, iy, 1);
                while (queue.size() > 0) {
                    int x = queue.front().x;
                    int y = queue.front().y;
                    queue.pop_front();
                    tryToAdd(queue, x-1, y, map[mx*y+x]+1);
                    tryToAdd(queue, x+1, y, map[mx*y+x]+1);
                    tryToAdd(queue, x, y-1, map[mx*y+x]+1);
                    tryToAdd(queue, x, y+1, map[mx*y+x]+1);
                }

                pathlen = map[mx*ey+ex] / 2;

                //Build one path
                {
                    int x = ex, y = ey;
                    while (true) {
                        if (map[mx*y+x] == pathlen + 1) {
                            meetingspot_x = x;
                            meetingspot_y = y;
                        }

                        if (tryToRegisterAsPath(x-1, y, &x, &y) || tryToRegisterAsPath(x+1, y, &x, &y) ||
                            tryToRegisterAsPath(x, y-1, &x, &y) || tryToRegisterAsPath(x, y+1, &x, &y))
                            ; // registered

                        // Block coords updated so we can check for path's ending and finish it
                        if (map[mx*y+x] == 1) {
                            map[mx*y+x] = -126;
                            break;
                        }
                    }
                }
            }

            Direction ivanDir = getNextStep(ix, iy);
            Direction elenaDir = getNextStep(ex, ey);

            // Sync positions (for map rendering)
            ivan.posx = ix - istartx;
            ivan.posy = iy - istarty;
            elena.posx = ex - estartx;
            elena.posy = ey - estarty;

            found = world.go(ivanDir, elenaDir);
            if (found) {
                if (ix == meetingspot_x && iy == meetingspot_y &&
                     ex == meetingspot_x && ey == meetingspot_y) {
                     std::cout << std::endl << "Ffound. Used " << world.getTurnCount() << " turns. Final map:" << std::endl;
                    int mapsizex = mx;
                    int mapsizey = my;
                    int ys = 0;
                    int xs = 0;
                    if (mapsizex == 12) {
                        mapsizex = 10;
                        xs += 2;
                    }
                    if (mapsizey == 12) {
                        mapsizey = 10;
                        ys += 2;
                    }
                    for (int y = ys; y < my; ++y) {
                        for (int x = xs; x < mx; ++x) {
                            if (map[mx*y+x] == -128) std::cout << '?';
                            else if (map[mx*y+x] == -127) std::cout << '#';
                            else {
                                if (x == istartx && y == istarty)
                                    std::cout << '@';
                                else if (x == estartx && y == estarty)
                                    std::cout << '&';
                                else
                                    std::cout << '.';
                            }
                        }
                        std::cout << std::endl;
                    }
                    return 0;
                 }
                 // normally would not happen:

                 //something went wrong if they met before getting together at meeting spot...
                 exploreEverything = false;
                 //ok, jumping to "unexpected" meeting handler
            }
        } else if (ivan.countOfUnexploredBlocks == 0 && ivan.countOfExploredBlocks < elena.countOfExploredBlocks + elena.countOfUnexploredBlocks ||
            elena.countOfUnexploredBlocks == 0 && elena.countOfExploredBlocks < ivan.countOfExploredBlocks + ivan.countOfUnexploredBlocks) {
            std::cout << "They will never meet. They're in different \"rooms\" (one room is smaller than other by volume). Turns used: " << world.getTurnCount() << "." << std::endl;
            printMaps(ivan, elena);
            return 0;
        } else // just exploring
            found = world.go(doCharTurn(ivan), doCharTurn(elena));
        if (!exploreEverything && found) {
            std::cout << std::endl << "Found. Used " << world.getTurnCount() << " turns. Final map:" << std::endl;

            // Make these two stand in one block
            bool result = false;
            if (world.canGo(Character::Ivan, Direction::Up)) {
                result = world.go(Direction::Up, Direction::Pass);
                if (result)
                    ivan.posy -= 1;
                else
                    result = world.go(Direction::Down, Direction::Pass);
            }
            if (!result) {
                if (world.canGo(Character::Ivan, Direction::Down)) {
                    result = world.go(Direction::Down, Direction::Pass);
                    if (result)
                        ivan.posy += 1;
                    else
                        world.go(Direction::Up, Direction::Pass);
                }
                if (!result) {
                    if (world.canGo(Character::Ivan, Direction::Left)) {
                        result = world.go(Direction::Left, Direction::Pass);
                        if (result)
                            ivan.posx -= 1;
                        else
                            world.go(Direction::Right, Direction::Pass);
                    }
                    if (!result) {
                        if (world.canGo(Character::Ivan, Direction::Right)) {
                            result = world.go(Direction::Right, Direction::Pass);
                            if (result)
                                ivan.posx += 1;
                            else // normally would not happen
                                world.go(Direction::Left, Direction::Pass);
                        }
                    }
                }
            }

            syncMaps(ivan, elena);
            *ivan.at(ivan.posx - elena.posx, ivan.posy - elena.posy) = '&';
            printMap(ivan);
            return 0;
        }
    }

    return 0;
}
