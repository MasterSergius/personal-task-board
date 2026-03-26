class AppState:
    def __init__(self):
        self.projects = {}
        self.current_project = None

        # Sample projects
        self.projects = {
            "Game Dev": [
                {"title": "To Do", "cards": ["Player movement", "Enemy AI"]},
                {"title": "Doing", "cards": ["Inventory system"]},
                {"title": "Blocked", "cards": ["Multiplayer sync"]},
                {"title": "Done", "cards": ["Project setup"]},
            ],
            "Writing": [
                {"title": "Ideas", "cards": ["Scene 1", "Scene 2"]},
                {"title": "Drafting", "cards": ["Chapter 1"]},
                {"title": "Revisions", "cards": []},
            ],
        }

    # ------------------------
    # Project management
    # ------------------------

    def add_project(self, name):
        if name not in self.projects:
            self.projects[name] = []
            self.current_project = name

    def delete_project(self, name):
        if name in self.projects:
            del self.projects[name]

            if self.current_project == name:
                self.current_project = next(iter(self.projects), None)

    def get_projects(self):
        return list(self.projects.keys())

    def get_current_columns(self):
        if self.current_project:
            return self.projects[self.current_project]
        return []


app_state = AppState()
